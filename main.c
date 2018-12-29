#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/soundcard.h>
#include <ctype.h>

#include "setbaud.h"
#include "serial.h"
#include "config.h"
#include "misc.h"
#include "udpsock.h"
#include "tcpsock.h"
#include "alsa.h"
#include "ini.h"

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
// void show_debug_buf(char * descr, char * buf, int bufLen) 
//
void show_debug_buf(char * descr, char * buf, int bufLen)
{
    if(MIDI_DEBUG)
    {
        misc_print("%s[%02d] -->", descr, bufLen);
        for (unsigned char * byte = buf; bufLen-- > 0; byte++)
            misc_print(" %02x", *byte);
        misc_print("\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * socket_thread_function(void * x)
// Thread function for /dev/midi input
//
void * tcpsock_thread_function (void * x)
{
    unsigned char buf[100];
    int rdLen;
    do
    {
        rdLen = tcpsock_read(socket_out, (char *) buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            show_debug_buf("TSOCK IN ", buf, rdLen);            
        }
    } while (socket_out != -1);
    if(MIDI_DEBUG)
       misc_print("TCPSOCK Thread fuction exiting.\n", socket_out);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * udpsock_thread_function(void * x)
// Thread function for /dev/midi input
//
void * udpsock_thread_function (void * x)
{
    unsigned char buf[100];
    int rdLen;
    do
    {
        rdLen = udpsock_read(socket_in,  (char *) buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            show_debug_buf("USOCK IN ", buf, rdLen);            
        }
    } while (TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// do_check_modem_hangup(char * buf, int bufLen) 
//  
//
void do_check_modem_hangup(char * buf, int bufLen)
{
    static char lineBuf[8];    
    static char iLineBuf = 0;
    static char lastChar = 0x00;
    char tmp[100] = "";
    
    for (char * p = buf; bufLen-- > 0; p++)
    {
        //*p = toupper(*p);
        switch(*p)
        {
        case 0x0d:// [RETURN]
            if(memcmp(lineBuf, "+++ATH", 6) == 0)
            {
                close(socket_out);
                socket_out = -1;
                sleep(1);
                sprintf(tmp, "\r\nHANG-UP DETECTED\r\n");
                if(MIDI_DEBUG)
                    misc_print("HANG-UP Detected.\n");
                write(fdSerial, tmp, strlen(tmp));
                write(fdSerial, "OK\r\n", 4);                
            }          
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
            lastChar = 0x0d;
            break;
        case '+': // RESET BUFFER
            if (lastChar != '+')
                iLineBuf = 0;
        default:  
            if (iLineBuf < sizeof(lineBuf)-1)
            {
                lineBuf[iLineBuf++] = *p;
                lineBuf[iLineBuf] = 0x00;
            }
            lastChar = *p;
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void do_modem_emulation(char * buf, int bufLen)
//
//
void do_modem_emulation(char * buf, int bufLen)
{
    static char lineBuf[150] = "";
    static char iLineBuf = 0;
    char tmp[100]  = "";
    char * endPtr;
    
    show_debug_buf("SER OUT  ", buf, bufLen);
    for (char * p = buf; bufLen-- > 0; p++)
    {
        *p = toupper(*p);
        switch(*p)
        {
        case ' ':  // [SPACE] no space
            break;
        case 0x0D: // [RETURN]
            if(memcmp(lineBuf, "ATDT", 4) == 0)
            {
                char * colon  = strchr(lineBuf, ':');
                char * port   = (colon == NULL)?NULL:(colon + 1);
                char * ipAddr = &lineBuf[4];
                if (colon != NULL) *colon = 0x00;
                if (strlen(ipAddr) < 3)
                {
                    char example [] = "\r\nEXAMPLE --> ATDT192.168.1.100:1999\r\n";
                    write(fdSerial, example, strlen(example));
                }
                else
                {
                    int iPort = (port == NULL)?23:strtol(port, &endPtr, 10);
                    if (!misc_is_ip_addr(ipAddr))
                        misc_hostname_to_ip(ipAddr, ipAddr);
                    sprintf(tmp, "\r\nDIALING %s:%d", ipAddr, iPort);
                    write(fdSerial, tmp, strlen(tmp));
                    socket_out = tcpsock_client_connect(ipAddr, iPort, fdSerial);
                    if(socket_out > 0)
                    {
                        sprintf(tmp, "\r\nCONNECTED %d\r\n", midiServerBaud);
                        write(fdSerial, tmp, strlen(tmp));
                        int status = pthread_create(&socketInThread, NULL, tcpsock_thread_function, NULL);
                    }
                    else
                        write(fdSerial, "\r\nOK\r\n", 6);
                }
            }
            else if (memcmp(lineBuf, "ATBAUD", 6) == 0)
            {
                char * baud = &lineBuf[6];
                int iBaud = strtol(baud, &endPtr, 10);
                if (setbaud_is_valid_rate (iBaud))
                {
                    int sec = 10;
                    sprintf(tmp, "\r\nSetting BAUD to %d in %d seconds...", iBaud, sec);
                    write(fdSerial, tmp, strlen(tmp));
                    sleep(sec);
                    setbaud_set_baud(serialDevice, fdSerial, iBaud);
                    midiServerBaud = iBaud;
                    sprintf(tmp, "\r\nBAUD has been set to %d", iBaud);
                    write(fdSerial, tmp, strlen(tmp));
                }
                else
                {
                    sprintf(tmp, "\r\nBAUD rate '%d' is not valid.\r\n", iBaud);
                    write(fdSerial, tmp, strlen(tmp));
                }
                write(fdSerial, "\r\nOK\r\n", 6);
            } 
            else if (memcmp(lineBuf, "ATIP", 4) == 0)
            {
                sprintf(tmp, "\r\n---------------------\r\n");
                write(fdSerial, tmp, strlen(tmp));
                misc_get_ipaddr("eth0", tmp);
                write(fdSerial, " ", 1);
                write(fdSerial, tmp, strlen(tmp));
                write(fdSerial, "\r\n", 2);
                misc_get_ipaddr("wlan0", tmp);
                write(fdSerial, tmp, strlen(tmp));
                write(fdSerial, "\r\nOK\r\n", 6);
            }
            else
            {
                write(fdSerial, "\r\n", 2);
                write(fdSerial, lineBuf, iLineBuf);
                write(fdSerial, "\r\n", 2);
            }
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
            break;
        case 0x08: // [DELETE]
        case 0xf8: // [BACKSPACE]
            if (iLineBuf > 0)
            {
                lineBuf[--iLineBuf] = 0x00;
                write(fdSerial, p, 1);
            }
            break;
        default:
            if (iLineBuf < 80)
            {
                lineBuf[iLineBuf++] = *p;
                write(fdSerial, p, 1);
                lineBuf[iLineBuf] = 0x00;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * midi_thread_function(void * x)
// Thread function for /dev/midi input
//
void * midi_thread_function (void * x)
{
    unsigned char buf [100];
    int rdLen;
    do
    {
        rdLen = read(fdMidi, &buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            show_debug_buf("MIDI IN  ", buf, rdLen);
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
    show_debug_buf("MIDI OUT ", buf, bufLen);
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
            show_debug_buf("MIDI1 IN ", buf, rdLen);
        }
    } while (TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_socket_packet()
// this is for TCP/IP
//
void write_socket_packet(char * buf, int bufLen, int TCP)
{
    if (TCP)
        tcpsock_write(socket_out, buf, bufLen);
    else
        udpsock_write(socket_out, buf, bufLen);

    show_debug_buf("SOCK OUT ", buf, bufLen);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_[sequencer]_packet()
// this is for ALSA sequencer interface
//
void write_alsa_packet(char * buf, int bufLen)
{
    alsa_send_midi_raw(buf, bufLen);
    show_debug_buf("SEQU OUT ", buf, bufLen);
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
    int TCP    = FALSE;

    if (misc_check_args_option(argc, argv, "AUTO") && !misc_check_device(midiDevice))
    {
        if (misc_check_file("/tmp/ML_MUNT"))   MUNT   = TRUE;
        if (misc_check_file("/tmp/ML_MUNTGM")) MUNTGM = TRUE;
        if (misc_check_file("/tmp/ML_FSYNTH")) FSYNTH = TRUE;
        if (misc_check_file("/tmp/ML_UDP"))    UDP    = TRUE;
        if (misc_check_file("/tmp/ML_TCP"))    TCP    = TRUE;
        if (!MUNT && !MUNTGM && !FSYNTH && !UDP)
        {
            misc_print("AUTO --> TCP\n");
            TCP = TRUE;
        }
    }
    else
    {
        MUNT   = misc_check_args_option(argc, argv, "MUNT");
        MUNTGM = misc_check_args_option(argc, argv, "MUNTGM");
        FSYNTH = misc_check_args_option(argc, argv, "FSYNTH");
        UDP    = misc_check_args_option(argc, argv, "UDP");
        TCP    = misc_check_args_option(argc, argv, "TCP");
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

    serial_set_interface_attribs(fdSerial);

    if (UDP && midiServerBaud != -1)
    {
        //do nothing. 
    }
    else if (TCP)
    {
        midiServerBaud = 115200;
    }
    else
    {
        if (misc_check_args_option(argc, argv, "38400"))
            midiServerBaud = 38400;
        else
            midiServerBaud = 31250;
    }

    setbaud_set_baud(serialDevice, fdSerial, midiServerBaud);

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
        {   
            close_fd();
            return -2;
        }
        close_fd();
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
            close_fd();
            return -4;
        }
    }
    else if (!TCP)
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            misc_print("ERROR: cannot open %s: %s\n", midiDevice, strerror(errno));
            close_fd();
            return -5;
        }

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
        if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
        {
            misc_print("Testing --> %s\n", midiDevice);
            test_midi_device();
        }

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
            {
                write_socket_packet(buf, rdLen, TCP);
                if (TCP == TRUE)
                    do_check_modem_hangup(buf, rdLen);
            }
            else if (TCP == TRUE)
                do_modem_emulation(buf, rdLen);
        }
        else if (rdLen < 0)
            misc_print("ERROR: from read: %d: %s\n", rdLen, strerror(errno));
    } while (TRUE);

    close_fd();
    return 0;
}