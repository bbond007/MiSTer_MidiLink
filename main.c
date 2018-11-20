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
#include "socket.h"
#include "alsa.h"
#include "ini.h"
#define TRUE 1
#define FALSE 0

static int		MIDI_DEBUG	       = TRUE;
static int		fdSerial	       = -1;
static int		fdMidi		       = -1;
static int		fdMidi1		       = -1;
static int 		socket 		       = -1;
char         		fsynthSoundFont [150]  = "/media/fat/SOUNDFONT/default.sf2";
char         		midiServer [50]        = "";
int 			muntVolume             = -1;
int 			fsynthVolume           = -1;
int 			midilinkPriority       = 0;
unsigned int 		midiServerPort         = 1999;
static pthread_t	midiInThread;
static pthread_t	midi1InThread;
static pthread_t	socketInThread;

///////////////////////////////////////////////////////////////////////////////////////
//
// void * midi_thread_function(void * x)
// Thread function for /dev/midi input
//
void * midi_thread_function (void * x)
{
    unsigned char inbytes [100];
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
           if (MIDI_DEBUG)
                printf("MIDI IN  [%02d]-->", rdLen);

            for (inbyte = inbytes; rdLen-- > 0; inbyte++)
            {
                if (MIDI_DEBUG)
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
    if (MIDI_DEBUG)
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
// Play a test note : this is for /dev/midi
//
void test_midi_device()
{
    write_midi_packet(test_note, sizeof(test_note));
}

///////////////////////////////////////////////////////////////////////////////////////
//
// midi1in_thread_function
// Thread function for /dev/midi1 input
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
            if (MIDI_DEBUG)
                printf("MIDI1 IN [%02d] -->", rdLen);
            for (inbyte = inbytes; rdLen-- > 0; inbyte++)
            {
                if (MIDI_DEBUG)
                    printf(" %02x", *inbyte);
                write(fdSerial, inbyte, 1);
                write(fdMidi, inbyte, 1);
            }
            if (MIDI_DEBUG)
                printf("\n");
        }
    } while (1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_socket_packet()
// this is for TCP/IP
//
void write_socket_packet(char * buf, int bufLen)
{
    socket_write(socket, buf, bufLen);
    if (MIDI_DEBUG)
    {
        printf("SOCK OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            printf(" %02x", *p);
        printf("\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_[sequencer]_packet()
// this is for ALSA sequencer interface
//
void write_alsa_packet(char * buf, int bufLen)
{
    alsa_send_midi_raw(buf, bufLen);
    if (MIDI_DEBUG)
    {     
        printf("SEQU OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            printf(" %02x", *p);
        printf("\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void show_line()
//
void show_line()
{
    if (MIDI_DEBUG)
    {
        printf("MIDI debug messages enabled.\n");
        printf("---------------------------------------------\n\n");
    }
    else
        printf("QUIET mode emabled.\n");
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void set_pcm_valume
//
void set_pcm_volume(int value)
{
    if(value != -1)
    {
        char buf[30];
        sprintf(buf, "amixer set PCM %d%c", value, '%');
        printf("Setting 'PCM' to %d%\n", value);
        system(buf);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void close_fd()
//
void close_fd()
{
    if (fdSerial > 0) close (fdSerial);
    if (fdMidi > 0)   close (fdMidi);
    if (fdMidi1 > 0)  close (fdMidi1);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{	
    unsigned char buf[256];
    
    printf("\e[2J\e[H"); 
    printf(helloStr);
    if(misc_check_file(midiLinkINI))
        ini_read_ini(midiLinkINI);
   
    if (misc_check_args_option(argc, argv, "QUIET"))
        MIDI_DEBUG = FALSE;
    else
        MIDI_DEBUG = TRUE;
    
    if (midilinkPriority != 0)
        misc_set_priority(midilinkPriority);
             
    int MUNT   = FALSE;
    int MUNTGM = FALSE;
    int FSYNTH = FALSE; 
    int UDP    = FALSE; 
    
    if (misc_check_args_option(argc, argv, "AUTO") && !misc_check_device(midiDevice)) 
    {
        if (misc_check_file("/tmp/ML_MUNT"))   MUNT   = TRUE;
        if (misc_check_file("/tmp/ML_MUNTGM")) MUNTGM = TRUE;
        if (misc_check_file("/tmp/ML_FSYNTH")) FSYNTH = TRUE;
        if (misc_check_file("/tmp/ML_UDP"))    UDP    = TRUE;
        if (!MUNT && !MUNTGM && !FSYNTH && !UDP)
        { 
            printf("AUTO --> MUNT\n");
            MUNT = TRUE;
        }
    }
    else
    {
        MUNT   = misc_check_args_option(argc, argv, "MUNT");
        MUNTGM = misc_check_args_option(argc, argv, "MUNTGM");
        FSYNTH = misc_check_args_option(argc, argv, "FSYNTH"); 
        UDP    = misc_check_args_option(argc, argv, "UDP"); 
    }
    
    printf("Killing --> fluidsynth\n");
    system("killall -q fluidsynth");
    printf("Killing --> mt32d\n");
    system("killall -q mt32d");
    sleep(3);
        
    if (MUNT || MUNTGM)
    {
        set_pcm_volume(muntVolume);
        printf("Starting --> mt32d\n");
        system("mt32d &");
        sleep(2);
    }
    else if (FSYNTH)
    {
        set_pcm_volume(fsynthVolume);
        printf("Starting --> fluidsynth\n");
        sprintf(buf, "fluidsynth -is -a alsa -m alsa_seq %s &", fsynthSoundFont);
        system(buf);
        sleep(2);
    }
        
    fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdSerial < 0)
    {
        printf("Error opening %s: %s\n", serialDevice, strerror(errno));
        close_fd();
        return -1;
    }

    serial_set_interface_attribs(fdSerial); //SETS SERIAL PORT 38400 <-- perfect for ao486 / SoftMPU

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
            return -2;
        }
    }

    serial_do_tcdrain(fdSerial);
      
    if (MUNT || MUNTGM || FSYNTH)
    {        
        if(alsa_open_seq(128, MUNTGM) == 0)
        {
            show_line();
            do
            {
                int rdLen = read(fdSerial, buf, sizeof(buf));
                if (rdLen > 0)
                {
                    write_alsa_packet(buf, rdLen); 
                }
                else if (rdLen < 0)
                    printf("Error from read: %d: %s\n", rdLen, strerror(errno));
            } while (1);
        }
        else
            return -2;
            
        return 0;
    }

    if (UDP)
    {
        if(misc_check_file(midiLinkINI) && strlen(midiServer) > 7)
        {
            printf("Connecting to server --> %s:%d\n",midiServer, midiServerPort);
            socket = socket_client_connect(midiServer, midiServerPort);
        }   
        else
        {
            printf("ERROR in INI File --> %s\n", midiLinkINI);
            return -4;
        }            
    }
    else
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            printf("Error: cannot open %s: %s\n", midiDevice, strerror(errno));
            close_fd();
            return -5;
        }

#ifdef MIDI_INPUT
        //if (misc_check_args_option(argc, argv, "MIDI1")
        if (misc_check_device(midi1Device))
        {
            fdMidi1 = open(midi1Device, O_RDONLY);
            if (fdMidi1 < 0)
            {
                printf("Error: cannot open %s: %s\n", midi1Device, strerror(errno));
                close_fd();
                return -6;
            }
        }
#endif
        if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
        {
            printf("Testing --> %s\n", midiDevice);
            test_midi_device();
        }

#ifdef MIDI_INPUT
        if (fdMidi1 != -1)
        {
            int statusM1 = pthread_create(&midi1InThread, NULL, midi1in_thread_function, NULL);
            if (statusM1 == -1)
            {
                printf("Error: unable to create *MIDI input thread.\n");
                close_fd();
                return -7;
            }
            printf("MIDI1 input thread created.\n");
            printf("CONNECT : %s --> %s & %s\n", midi1Device, serialDevice, midiDevice);
        }

        int status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);
        if (status == -1)
        {
            printf("Error: unable to create MIDI input thread.\n");
            close_fd();
            return -8;
        }
        printf("MIDI input thread created.\n");
        printf("CONNECT : %s <--> %s\n", serialDevice, midiDevice);
#else
        printf("CONNECT : %s --> %s\n", serialDevice, midiDevice);
#endif
    }
  
    show_line();

    //  Main thread handles MIDI output
    do
    {
        int rdLen = read(fdSerial, buf, sizeof(buf));
        if (rdLen > 0)
        {
            if(fdMidi != -1)
                write_midi_packet(buf, rdLen);
            if(socket != -1)
                write_socket_packet(buf,rdLen);
        }
        else if (rdLen < 0)
            printf("Error from read: %d: %s\n", rdLen, strerror(errno));
    } while (1);

    close_fd();
    return 0;
}