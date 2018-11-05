#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/soundcard.h>
#include "setbaud.h"
#include "serial.h"
#include "config.h"
#include "misc.h"

#define TRUE 1
#define FALSE 0

static int             fdSerial		= -1;
static int             fdMidi		= -1;
static int             fdMidi1		= -1;
static pthread_t       midiInThread;
static pthread_t       midi1InThread;
static int 	       MIDI_DEBUG	= TRUE;

///////////////////////////////////////////////////////////////////////////////////////
//
// void * midi_thread_function(void * x) --> functions for running as a separate thread.
// this is for /dev/midi
#ifdef MIDI_DEV
void * midi_thread_function (void * x)
{
    unsigned char inbytes [100];             // bytes from sequencer driver
    unsigned char * inbyte;
    int rdLen;
    do
    {
        rdLen = read(fdMidi, &inbytes, sizeof(inbytes));
        if (rdLen < 0)
        {
            printf("Error reading %s --> %d : %s \n", midiDevice, rdLen, strerror(errno));
        }
        else
        {
            if(MIDI_DEBUG)
              printf("MIDI IN  [%02d]-->", rdLen);
              
            for (inbyte = inbytes; rdLen-- > 0; inbyte++)
            {
                if(MIDI_DEBUG)
                    printf(" %02x",*inbyte);
                write(fdSerial, inbyte, 1);
            }
            if (MIDI_DEBUG)
                printf("\n");
        }
    } while (1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_midi_packet()
// this is for /dev/midi
//
void write_midi_packet(char * buf, int bufLen)
{
    write(fdMidi, buf, bufLen);
    if(MIDI_DEBUG)
    {
        printf("MIDI OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            printf(" %02x", *p);
        printf("\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// test_midi_device()
// Play a note : this is for /dev/midi
//
void test_midi_device()
{
    write_midi_packet(test_note, sizeof(test_note));
}
#else
///////////////////////////////////////////////////////////////////////////////////////
//
// [sequencer]_thread_function --> functions for running as a separate thread.
// this is for /dev/sequencer
//
void* midi_thread_function (void* x)
{
    unsigned char inbytes[4];         // bytes from sequencer driver
    int rdLen;                        // for error checking
    do
    {
        do
        {
            rdLen = read(fdMidi, &inbytes, sizeof(inbytes));
            if (rdLen < 0)
            {
                printf("Error reading %s --> %d : %s \n", midiDevice, status, strerror(errno));
            }
            else if (inbytes[0] == SEQ_MIDIPUTC)
            {
                if(MIDI_DEBUG)
                    printf("SEQU IN --> %02x\n", inbytes[1]);
                write(fdSerial, &inbytes[1], 1);
            }
        } while (rdLen > 0);
    } while (1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_[sequencer]_packet()
// this is for /dev/sequencer
//
void write_midi_packet(char * buf, int bufLen)
{
    unsigned char midiPacket[4] = {SEQ_MIDIPUTC, 0, midiDevnum, 0};
    unsigned char  *p;
    if(MIDI_DEBUG)
        printf("MIDI1 IN [%02d] -->", bufLen);
    for (p = buf; bufLen-- > 0; p++)
    {
        if(MIDI_DEBUG)
            printf(" %02x", *p);
        midiPacket[1] = *p;
        write(fdMidi, midiPacket, sizeof(midiPacket));
    }
    if(MIDI_DEBUG)
        printf("\n");
}

///////////////////////////////////////////////////////////////////////////////////////
//
// test_midi_device()
// Play a note : this is for /dev/sequencer
//
void test_midi_device()
{
    unsigned char packet[4] = {SEQ_MIDIPUTC, 0, midiDevnum, 0};
    packet[1] = test_note[0];
    write(fdMidi, packet, sizeof(packet));
    packet[1] = test_note[1];
    write(fdMidi, packet, sizeof(packet));
    packet[1] = test_note[2];
    write(fdMidi, packet, sizeof(packet));
}
#endif

///////////////////////////////////////////////////////////////////////////////////////
//
// [sequencer]_thread_function --> functions for running as a separate thread.
// this is for /dev/midi1
//
void * midi1in_thread_function (void * x)
{
    unsigned char inbytes [100];             // bytes from sequencer driver
    unsigned char * inbyte;
    int rdLen;
    do
    {
        rdLen = read(fdMidi1, &inbytes, sizeof(inbytes));
        if (rdLen < 0)
        {
            printf("Error reading %s --> %d : %s \n", midi1Device, rdLen, strerror(errno));
            sleep(10);
            if (misc_check_device(midi1Device))
            {
                printf("Reopening  %s --> %d : %s \n", midi1Device);
                fdMidi1 = open(midi1Device, O_RDONLY);
            }
        }
        else
        {
            if(MIDI_DEBUG)
                printf("MIDI1 IN [%02d] -->", rdLen);
            for (inbyte = inbytes; rdLen-- > 0; inbyte++)
            {
                if(MIDI_DEBUG)
                    printf(" %02x", *inbyte);
                write(fdSerial, inbyte, 1);
                write(fdMidi, inbyte, 1);
            }
            if(MIDI_DEBUG)
                printf("\n");
        }
    } while (1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// close_fd()
//
void close_fd()
{
    if (fdSerial > 0) close(fdSerial);
    if (fdMidi > 0)   close (fdMidi);
    if (fdMidi1 > 0)  close (fdMidi1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{
    printf(helloStr);

    if (misc_check_args_option(argc, argv, "QUIET"))
        MIDI_DEBUG = FALSE;
    else
        MIDI_DEBUG = TRUE;
    fdMidi = open(midiDevice, O_RDWR);
    if (fdMidi < 0)
    {
        printf("Error: cannot open %s: %s\n", midiDevice, strerror(errno));
        close_fd();
        return -1;
    }


    fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdSerial < 0)
    {
        printf("Error opening %s: %s\n", serialDevice, strerror(errno));
        close_fd();
        return -3;
    }

    serial_set_interface_attribs(fdSerial); //SETS SERIAL PORT 38400 <-- perfect for ao486

    //if (misc_check_args_option(argc, argv, "MIDI1")
    if (misc_check_device(midi1Device))
    {

        fdMidi1 = open(midi1Device, O_RDONLY);
        if (fdMidi1 < 0)
        {
            printf("Error: cannot open %s: %s\n", midi1Device, strerror(errno));
            close_fd();
            return -2;
        }
    }

    if (!misc_check_args_option(argc, argv, "38400"))
        setbaud_set_baud(serialDevice, fdSerial, 31200);  // <-- not so good for Amiga where we need midi speed
    else
        printf("Setting %s to 38400 baud.\n",serialDevice);

    if (misc_check_args_option(argc, argv, "TESTSER")) //send hello message too serial port
    {
        int wlen = write(fdSerial, helloStr, strlen(helloStr));
        if (wlen != strlen(helloStr))
        {
            printf("Error from write: %d, %d\n", wlen, errno);
            close_fd();
            return -4;
        }
    }

    serial_do_tcdrain(fdSerial);
    if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
    {
        printf("Testing --> %s\n", midiDevice);
        test_midi_device();
    }

    if (fdMidi1 != -1)
    {
        int statusM1 = pthread_create(&midi1InThread, NULL, midi1in_thread_function, NULL);
        if (statusM1 == -1)
        {
            printf("Error: unable to create *MIDI input thread.\n");
            close_fd();
            return -5;
        }
        printf("MIDI1 input thread created.\n");
        printf("CONNECT : %s --> %s & %s\n", midi1Device, serialDevice, midiDevice);
    }

#ifdef MIDI_INPUT
    int status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);

    if (status == -1)
    {
        printf("Error: unable to create MIDI input thread.\n");
        close_fd();
        return -6;
    }
    printf("MIDI input thread created.\n");
    printf("CONNECT : %s <--> %s\n", serialDevice, midiDevice);
#else
    printf("CONNECT : %s --> %s\n", serialDevice, midiDevice);
#endif

    if(MIDI_DEBUG)
    {
        printf("MIDI debug messages enabled.\n");
        printf("---------------------------------------------\n\n");
    }
    else
        printf("QUIET mode emabled.\n");


    unsigned char buf[100];
    //  Main thread handles MIDI output
    do
    {
        int rdlen = read(fdSerial, buf, sizeof(buf));
        if (rdlen > 0)
            write_midi_packet(buf, rdlen);
        else if (rdlen < 0)
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
    } while (1);

    close_fd();
    return 0;
}