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
#include "udpsock.h"
#include "alsa.h"
#include "ini.h"
#define TRUE 1
#define FALSE 0

int                     MIDI_DEBUG	       = TRUE;
static int		fdSerial	       = -1;
static int		fdMidi		       = -1;
static int		fdMidi1		       = -1;
static int 		socket_in	       = -1;
static int 		socket_out	       = -1;
char         		fsynthSoundFont [150]  = "/media/fat/SOUNDFONT/default.sf2";
char         		midiServer [50]        = "";
int 			muntVolume             = -1;
int 			fsynthVolume           = -1;
int 			midilinkPriority       = 0;
int 			midiServerBaud	       = -1;
unsigned int 		midiServerPort         = 1999;
unsigned int            midiServerFilterIP     = FALSE;
static pthread_t	midiInThread;
static pthread_t	midi1InThread;
static pthread_t	socketInThread;

///////////////////////////////////////////////////////////////////////////////////////
//
// void * socket_thread_function(void * x)
// Thread function for /dev/midi input
//
void * udpsock_thread_function (void * x)
{
    unsigned char buf[100];
    unsigned char * byte;
    int rdLen;
    do 
    {
        rdLen = udpsock_read(socket_in, (char *) buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            if(MIDI_DEBUG)
            {
                misc_print("SOCK IN  [%02d] -->", rdLen);
                for (byte = buf; rdLen-- > 0; byte++)
                    misc_print(" %02x", *byte);
                misc_print("\n");
            }
        }        
    } while (TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * midi_thread_function(void * x)
// Thread function for /dev/midi input
//
void * midi_thread_function (void * x)
{
    unsigned char buf [100];
    unsigned char * byte;
    int rdLen;
    do
    {
        rdLen = read(fdMidi, &buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            if (MIDI_DEBUG)
            {
                misc_print("MIDI IN  [%02d]-->", rdLen);
                for (byte = buf; rdLen-- > 0; byte++)
                    misc_print(" %02x",*byte);                
                misc_print("\n");
            }
        }
        else
        {
            misc_print("ERROR: midi_thread_function() reading %s --> %d : %s \n", midiDevice, rdLen, strerror(errno));
        }
    } while (TRUE);
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
        misc_print("MIDI OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            misc_print(" %02x", *p);
        misc_print("\n");
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
    unsigned char buf[100];             // bytes from sequencer driver
    unsigned char * byte;
    int rdLen;
    do
    {
        rdLen = read(fdMidi1, &buf, sizeof(buf));
        if (rdLen < 0)
        {
            misc_print("ERROR: midi1in_thread_function() reading %s --> %d : %s \n", midi1Device, rdLen, strerror(errno));
            sleep(10);
            if (misc_check_device(midi1Device))
            {
                misc_print("Reopening  %s --> %d : %s \n", midi1Device);
                fdMidi1 = open(midi1Device, O_RDONLY);
            }
        }
        else
        {    
            write(fdSerial, buf, rdLen);
            write(fdMidi, buf, rdLen);
            if (MIDI_DEBUG)
            {
                misc_print("MIDI1 IN [%02d] -->", rdLen);
                for (byte = buf; rdLen-- > 0; byte++)
                    misc_print(" %02x", *byte);
                misc_print("\n");
            }
        }
    } while (TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_socket_packet()
// this is for TCP/IP
//
void write_socket_packet(char * buf, int bufLen)
{
    udpsock_write(socket_out, buf, bufLen);
    if (MIDI_DEBUG)
    {
        misc_print("SOCK OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            misc_print(" %02x", *p);
        misc_print("\n");
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
        misc_print("SEQU OUT [%02d] -->", bufLen);
        for (char * p = buf; bufLen-- > 0; p++)
            misc_print(" %02x", *p);
        misc_print("\n");
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
        misc_print("MIDI debug messages enabled.\n");
        misc_print("---------------------------------------------\n\n");
    }
    else
        misc_print("QUIET mode emabled.\n");
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
        misc_print("Setting 'PCM' to %d%\n", value);
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
    int status;
    unsigned char buf[256];
    
    misc_print("\e[2J\e[H"); 
    misc_print(helloStr);
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
            misc_print("AUTO --> UDP\n");
            UDP = TRUE;
        }
    }
    else
    {
        MUNT   = misc_check_args_option(argc, argv, "MUNT");
        MUNTGM = misc_check_args_option(argc, argv, "MUNTGM");
        FSYNTH = misc_check_args_option(argc, argv, "FSYNTH"); 
        UDP    = misc_check_args_option(argc, argv, "UDP"); 
    }
    
    misc_print("Killing --> fluidsynth\n");
    system("killall -q fluidsynth");
    misc_print("Killing --> mt32d\n");
    system("killall -q mt32d");
    sleep(3);
        
    if (MUNT || MUNTGM)
    {
        set_pcm_volume(muntVolume);
        misc_print("Starting --> mt32d\n");
        system("mt32d &");
        sleep(2);
    }
    else if (FSYNTH)
    {
        set_pcm_volume(fsynthVolume);
        misc_print("Starting --> fluidsynth\n");
        sprintf(buf, "fluidsynth -is -a alsa -m alsa_seq %s &", fsynthSoundFont);
        system(buf);
        sleep(2);
    }
        
    fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdSerial < 0)
    {
        misc_print("ERROR: opening %s: %s\n", serialDevice, strerror(errno));
        close_fd();
        return -1;
    }

    serial_set_interface_attribs(fdSerial); //SETS SERIAL PORT 38400 <-- perfect for ao486 / SoftMPU
    
    if (UDP && midiServerBaud > 0)
        setbaud_set_baud(serialDevice, fdSerial, midiServerBaud);
    else
    if (!misc_check_args_option(argc, argv, "38400"))
        setbaud_set_baud(serialDevice, fdSerial, 31200);  // <-- not so good for Amiga where we need midi speed
    else
        misc_print("Setting %s to 38400 baud.\n",serialDevice);

    serial_do_tcdrain(fdSerial);
      
    if (MUNT || MUNTGM || FSYNTH)
    {        
        if(alsa_open_seq(128, MUNTGM))
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
                    misc_print("ERROR: from read --> %d: %s\n", rdLen, strerror(errno));
            } while (TRUE);
        }
        else
            return -2;            
        return 0;
    }

    if (UDP)
    {
        if (strlen(midiServer) > 7)
        {
            misc_print("Connecting to server --> %s:%d\n",midiServer, midiServerPort);
            socket_out = udpsock_client_connect(midiServer, midiServerPort);
            socket_in  = udpsock_server_open(midiServerPort);
            if(socket_in > 0)
            {   
                misc_print("Socket Listener created on port %d.\n", midiServerPort);
                status = pthread_create(&socketInThread, NULL, udpsock_thread_function, NULL);
                if (status == -1)
                {
                    misc_print("ERROR: unable to create socket input thread.\n");
                    close_fd();
                    return -3;
                }
                misc_print("Socket input thread created.\n");
            }
        }   
        else
        {
            misc_print("ERROR: in INI File (MIDI_SERVER) --> %s\n", midiLinkINI);
            return -4;
        }            
    }
    else
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            misc_print("ERROR: cannot open %s: %s\n", midiDevice, strerror(errno));
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
                misc_print("ERROR: cannot open %s: %s\n", midi1Device, strerror(errno));
                close_fd();
                return -6;
            }
        }
#endif
        if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
        {
            misc_print("Testing --> %s\n", midiDevice);
            test_midi_device();
        }

#ifdef MIDI_INPUT
        if (fdMidi1 != -1)
        {
            status = pthread_create(&midi1InThread, NULL, midi1in_thread_function, NULL);
            if (status == -1)
            {
                misc_print("ERROR: unable to create *MIDI input thread.\n");
                close_fd();
                return -7;
            }
            misc_print("MIDI1 input thread created.\n");
            misc_print("CONNECT : %s --> %s & %s\n", midi1Device, serialDevice, midiDevice);
        }

        status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);
        if (status == -1)
        {
            misc_print("ERROR: unable to create MIDI input thread.\n");
            close_fd();
            return -8;
        }
        misc_print("MIDI input thread created.\n");
        misc_print("CONNECT : %s <--> %s\n", serialDevice, midiDevice);
#else
        misc_print("CONNECT : %s --> %s\n", serialDevice, midiDevice);
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
            if(socket_out != -1)
                write_socket_packet(buf,rdLen);
        }
        else if (rdLen < 0)
            misc_print("ERROR: from read: %d: %s\n", rdLen, strerror(errno));
    } while (TRUE);

    close_fd();
    return 0;
}