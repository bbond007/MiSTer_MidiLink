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

enum MODE {ModeNULL, ModeTCP, ModeUDP, ModeMUNT, ModeMUNTGM, ModeFSYNTH};

int                     MIDI_DEBUG	       = TRUE;
static enum MODE        mode                   = ModeNULL;
static int		fdSerial	       = -1;
static int		fdMidi		       = -1;
static int		fdMidi1		       = -1;
static int 		socket_in	       = -1;
static int 		socket_out	       = -1;
static int 		socket_lst             = -1;
static int 		baudRate	       = -1;
char         		fsynthSoundFont [150]  = "/media/fat/SOUNDFONT/default.sf2";
char         		UDPServer [50]         = "";
int 			muntVolume             = -1;
int 			fsynthVolume           = -1;
int 			midilinkPriority       = 0;
int                     UDPBaudRate            = -1;
int                     TCPBaudRate            = -1;
unsigned int            DELAYSYSEX	       = FALSE;
unsigned int 		UDPServerPort          = 1999;
unsigned int 		TCPServerPort          = 23;
unsigned int            UDPServerFilterIP      = FALSE;
static pthread_t	midiInThread;
static pthread_t	midi1InThread;
static pthread_t	socketInThread;
static pthread_t        socketLstThread;

///////////////////////////////////////////////////////////////////////////////////////
//
// void show_debug_buf(char * descr, char * buf, int bufLen)
//
void show_debug_buf(char * descr, char * buf, int bufLen)
{
    if(MIDI_DEBUG)
    {
        misc_print(2, "%s[%02d] -->", descr, bufLen);
        for (unsigned char * byte = buf; bufLen-- > 0; byte++)
            misc_print(2, " %02x", *byte);
        misc_print(2, "\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * midi_thread_function(void * x)
// Thread function for /dev/midi input
//
void * tcplst_thread_function (void * x)
{
    unsigned char buf[100];
    int rdLen;

    do
    {
        socket_in = tcpsock_accept(socket_lst);
        tcpsock_set_timeout(socket_in, 10);
        misc_print(1,"Incomming connection\n");
        tcpsock_get_ip(socket_in, buf);
        misc_print(1, "CONNECT --> %s\n", buf);
        if(socket_out == -1)
        {
            char ringStr[] = "\r\nRING";
            write(fdSerial, ringStr, strlen(ringStr));
            sprintf(buf, "\r\nCONNECT %d\r\n", baudRate);
            write(fdSerial, buf, strlen(buf));
            do
            {
                rdLen = read(socket_in, buf, sizeof(buf));
                //printf("rdLen --> %d\n", rdLen);
                if (rdLen > 0)
                {
                    write(fdSerial, buf, rdLen);
                    show_debug_buf("TSERV IN", buf, rdLen);
                }
                else if (rdLen == 0)
                {
                    tcpsock_close(socket_in);
                    socket_in = -1;
                    misc_print(1, "tcplst_thread_function() --> Connection Closed.\n");
                }
            } while (socket_in != -1);
        }
        else
        {
            char busyStr[] = "\r\nBUSY";
            misc_print(1, "Sending BUSY message and disconnecting.,\n");
            tcpsock_write(socket_in, busyStr, strlen(busyStr));
            sleep(2);
            tcpsock_close(socket_in);
            socket_in = -1;
        }
        
    } while(TRUE);
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
        else if(rdLen < 0)
                misc_print(1, "ERROR: tcpsock_thread_function() --> rdLen < 0\n");
    } while (socket_out != -1);
    if(MIDI_DEBUG)
        misc_print(1, "TCPSOCK Thread fuction exiting.\n", socket_out);
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
// void do_check_modem_hangup(char * buf, int bufLen)
//
//
void do_check_modem_hangup(int * socket, char * buf, int bufLen)
{
    static char lineBuf[8];
    static char iLineBuf = 0;
    static char lastChar = 0x00;
    char tmp[100] = "";

    for (char * p = buf; bufLen-- > 0; p++)
    {
        switch(*p)
        {
        case 0x0d:// [RETURN]
            if(memcmp(lineBuf, "+++ATH", 6) == 0)
            {
                tcpsock_close(*socket);
                *socket =  -1;
                sleep(1);
                sprintf(tmp, "\r\nHANG-UP DETECTED\r\n");
                misc_print(1, "HANG-UP Detected.\n");
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
                char * prtSep  = strchr(lineBuf, ':');
                if(prtSep == NULL) 
                    prtSep = strchr(lineBuf, '*'); // with NCOMM? 
                char * port   = (prtSep == NULL)?NULL:(prtSep + 1);
                char * ipAddr = &lineBuf[4];
                if (prtSep != NULL) *prtSep = 0x00;
                if (strlen(ipAddr) < 3)
                {
                    char serror   [] = "\r\nSyntax Error";
                    char line[] = "\r\n----------------------------------";
                    char example1 [] = "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG";
                    char example2 [] = "\r\nEXAMPLE --> ATDT192.168.1.100:1999";
                    char example3 [] = "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG*1999\r\nOK\r\n";
                    write(fdSerial, serror,   strlen(serror));
                    write(fdSerial, line, sizeof(line));
                    write(fdSerial, example1, strlen(example1));
                    write(fdSerial, example2, strlen(example2));
                    write(fdSerial, example3, strlen(example3));
                }
                else
                {
                    int ipError = FALSE;
                    int iPort = (port == NULL)?23:strtol(port, &endPtr, 10);
                    if (!misc_is_ip_addr(ipAddr))
                        if(!misc_hostname_to_ip(ipAddr, ipAddr))
                        {
                            sprintf(tmp, "\r\nERROR: Unable to convert hostname '%s' to IP address.", ipAddr);
                            write(fdSerial, tmp, strlen(tmp));
                            ipError = TRUE;
                        }
                    if(!ipError)
                    {
                        sprintf(tmp, "\r\nDIALING %s:%d", ipAddr, iPort);
                        write(fdSerial, tmp, strlen(tmp));
                        socket_out = tcpsock_client_connect(ipAddr, iPort, fdSerial);
                    }
                    if(socket_out > 0)
                    {
                        sprintf(tmp, "\r\nCONNECT %d\r\n", baudRate);
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
                int iBaud   = strtol(baud, &endPtr, 10);
                int iTemp   = setbaud_baud_at_index(iBaud);
                iBaud = (misc_is_number(baud) && iTemp > 0)?iTemp:iBaud;
                if (setbaud_is_valid_rate (iBaud))
                {
                    int sec = 10;
                    sprintf(tmp, "\r\nSetting BAUD to %d in %d seconds...", iBaud, sec);
                    write(fdSerial, tmp, strlen(tmp));
                    sleep(sec);
                    setbaud_set_baud(serialDevice, fdSerial, iBaud);
                    baudRate = iBaud;
                    sprintf(tmp, "\r\nBAUD has been set to %d", iBaud);
                    write(fdSerial, tmp, strlen(tmp));
                }
                else
                {
                    if(baud[0] != 0x00)
                        sprintf(tmp, "\r\nBAUD rate '%s' is not valid.", baud);
                    write(fdSerial, tmp, strlen(tmp));
                    setbaud_show_menu(fdSerial);
                }
                write(fdSerial, "\r\nOK\r\n", 6);
            }
            else if (memcmp(lineBuf, "ATIP", 4) == 0)
            {
                sprintf(tmp, "\r\n-------------------------\r\n");
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
                //write(fdSerial, lineBuf, iLineBuf);
                //write(fdSerial, "\r\n", 2);
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
            misc_print(1, "ERROR: midi_thread_function() reading %s --> %d : %s \n", midiDevice, rdLen, strerror(errno));
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
    static int SYSEX = FALSE;
    if (DELAYSYSEX) // This is for MT-32 Rev0 - 
    {               // spoon-feed the SYSEX data and delay after  
        misc_print(2, "MIDI OUT [%02d] -->", bufLen);
        for (unsigned char * byte = buf; bufLen-- > 0; byte++)
        {
            switch (*byte)
            {
                case 0xF0: // SYSEX START
                    SYSEX = TRUE;
                    misc_print(2, " SXD+ f0");
                    write(fdMidi, byte, 1);
                    usleep(100);	   
                    break;
            	case 0xF7: // SYSEX END
                    SYSEX = FALSE;
                    misc_print(2, " f7 SXD-");	   
            	    write(fdMidi, byte, 1);
            	    usleep(20000);
            	    break;
                default:
                    misc_print(2, " %02x", *byte);
                    write(fdMidi, byte, 1);
                    if (SYSEX) usleep(100);
                    break;
            }
        }
        misc_print(2, "\n");
    }
    else
    {
        write(fdMidi, buf, bufLen);
        show_debug_buf("MIDI OUT ", buf, bufLen);
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
    int rdLen;
    do
    {
        rdLen = read(fdMidi1, &buf, sizeof(buf));
        if (rdLen < 0)
        {
            misc_print(1, "ERROR: midi1in_thread_function() reading %s --> %d : %s \n", midi1Device, rdLen, strerror(errno));
            sleep(10);
            if (misc_check_device(midi1Device))
            {
                misc_print(1, "Reopening  %s --> %d : %s \n", midi1Device);
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
void write_socket_packet(int sock, char * buf, int bufLen)
{
    if (mode == ModeTCP)
        tcpsock_write(sock, buf, bufLen);
    else
        udpsock_write(sock, buf, bufLen);

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
        misc_print(0, "MIDI debug messages enabled.\n");
        misc_print(0, "---------------------------------------------\n\n");
    }
    else
        misc_print(0, "QUIET mode emabled.\n");
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
        misc_print(0, "Setting 'PCM' to %d%\n", value);
        system(buf);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void close_fd()
//
void close_fd()
{
    if (fdSerial   > 0) close (fdSerial);
    if (fdMidi     > 0) close (fdMidi);
    if (fdMidi1    > 0) close (fdMidi1);
    if (socket_in  > 0) tcpsock_close(socket_in);
    if (socket_out > 0) tcpsock_close(socket_out);
    if (socket_out > 0) tcpsock_close(socket_lst);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{
    int status;
    unsigned char buf[256];

    misc_print(0, "\e[2J\e[H");
    misc_print(0, helloStr);
    if(misc_check_file(midiLinkINI))
        ini_read_ini(midiLinkINI);

    if (misc_check_args_option(argc, argv, "QUIET"))
        MIDI_DEBUG = FALSE;
    else
        MIDI_DEBUG = TRUE;

    if (midilinkPriority != 0)
        misc_set_priority(midilinkPriority);

    if (misc_check_args_option(argc, argv, "MENU") && !misc_check_device(midiDevice))
    {
        if (misc_check_file("/tmp/ML_MUNT"))   mode   = ModeMUNT;
        if (misc_check_file("/tmp/ML_MUNTGM")) mode   = ModeMUNTGM;
        if (misc_check_file("/tmp/ML_FSYNTH")) mode   = ModeFSYNTH;
        if (misc_check_file("/tmp/ML_UDP"))    mode   = ModeUDP;
        if (misc_check_file("/tmp/ML_TCP"))    mode   = ModeTCP;
        if (mode != ModeMUNT && mode != ModeMUNTGM && mode != ModeFSYNTH &&
                mode != ModeTCP && mode != ModeUDP)
        {
            misc_print(0, "MENU --> TCP\n");
            mode = ModeTCP;
        }
    }
    else
    {
        if(misc_check_args_option(argc, argv, "MUNT"))   mode = ModeMUNT;
        if(misc_check_args_option(argc, argv, "MUNTGM")) mode = ModeMUNTGM;
        if(misc_check_args_option(argc, argv, "FSYNTH")) mode = ModeFSYNTH;
        if(misc_check_args_option(argc, argv, "UDP"))    mode = ModeUDP;
        if(misc_check_args_option(argc, argv, "TCP"))    mode = ModeTCP;
    }

    misc_print(0, "Killing --> fluidsynth\n");
    system("killall -q fluidsynth");
    misc_print(0, "Killing --> mt32d\n");
    system("killall -q mt32d");
    sleep(3);

    if (mode == ModeMUNT || mode == ModeMUNTGM)
    {
        set_pcm_volume(muntVolume);
        misc_print(0, "Starting --> mt32d\n");
        system("mt32d &");
        sleep(2);
    }
    else if (mode == ModeFSYNTH)
    {
        set_pcm_volume(fsynthVolume);
        misc_print(0, "Starting --> fluidsynth\n");
        sprintf(buf, "fluidsynth -is -a alsa -m alsa_seq %s &", fsynthSoundFont);
        system(buf);
        sleep(2);
    }

    fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdSerial < 0)
    {
        misc_print(0, "ERROR: opening %s: %s\n", serialDevice, strerror(errno));
        close_fd();
        return -1;
    }

    serial_set_interface_attribs(fdSerial);

    if (mode == ModeUDP && UDPBaudRate != -1)
    {
        baudRate = UDPBaudRate;
    }
    else if (mode == ModeTCP)
    {
        if(TCPBaudRate != -1)
            baudRate = TCPBaudRate;
        else
            baudRate = 115200;
    }
    else
    {
        if (misc_check_args_option(argc, argv, "38400"))
            baudRate = 38400;
        else
            baudRate = 31250;
    }

    setbaud_set_baud(serialDevice, fdSerial, baudRate);

    serial_do_tcdrain(fdSerial);

    if (mode == ModeMUNT || mode == ModeMUNTGM || mode == ModeFSYNTH)
    {
        if(alsa_open_seq(128, (mode == ModeMUNTGM)?1:0))
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
                    misc_print(0, "ERROR: from read --> %d: %s\n", rdLen, strerror(errno));
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

    if (mode == ModeUDP)
    {
        if (strlen(UDPServer) > 7)
        {
            misc_print(0, "Connecting to server --> %s:%d\n", UDPServer, UDPServerPort);
            socket_out = udpsock_client_connect(UDPServer, UDPServerPort);
            socket_in  = udpsock_server_open(UDPServerPort);
            if(socket_in > 0)
            {
                misc_print(0, "Socket Listener created on port %d.\n", UDPServerPort);
                status = pthread_create(&socketInThread, NULL, udpsock_thread_function, NULL);
                if (status == -1)
                {
                    misc_print(0, "ERROR: unable to create socket input thread.\n");
                    close_fd();
                    return -3;
                }
                misc_print(0, "Socket input thread created.\n");
            }
        }
        else
        {
            misc_print(0, "ERROR: in INI File (MIDI_SERVER) --> %s\n", midiLinkINI);
            close_fd();
            return -4;
        }
    }
    else if (mode == ModeTCP)
    {
        socket_lst = tcpsock_server_open(TCPServerPort);
        status = pthread_create(&socketLstThread, NULL, tcplst_thread_function, NULL);
        if (status == -1)
        {
           misc_print(0, "ERROR: unable to create socket listener thread.\n");
           close_fd();
           return -5;
        }
        misc_print(0, "Socket listener thread created.\n");
    }
    else
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            misc_print(0, "ERROR: cannot open %s: %s\n", midiDevice, strerror(errno));
            close_fd();
            return -6;
        }

        //if (misc_check_args_option(argc, argv, "MIDI1")
        if (misc_check_device(midi1Device))
        {
            fdMidi1 = open(midi1Device, O_RDONLY);
            if (fdMidi1 < 0)
            {
                misc_print(0, "ERROR: cannot open %s: %s\n", midi1Device, strerror(errno));
                close_fd();
                return -7;
            }
        }
        if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
        {
            misc_print(0, "Testing --> %s\n", midiDevice);
            test_midi_device();
        }

        if (fdMidi1 != -1)
        {
            status = pthread_create(&midi1InThread, NULL, midi1in_thread_function, NULL);
            if (status == -1)
            {
                misc_print(0, "ERROR: unable to create *MIDI input thread.\n");
                close_fd();
                return -8;
            }
            misc_print(0, "MIDI1 input thread created.\n");
            misc_print(0, "CONNECT : %s --> %s & %s\n", midi1Device, serialDevice, midiDevice);
        }

        status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);
        if (status == -1)
        {
            misc_print(0, "ERROR: unable to create MIDI input thread.\n");
            close_fd();
            return -9;
        }
        misc_print(0, "MIDI input thread created.\n");
        misc_print(0, "CONNECT : %s <--> %s\n", serialDevice, midiDevice);
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
            if(mode == ModeTCP && socket_in != -1)
            {
                do_check_modem_hangup(&socket_in, buf, rdLen);
                if(socket_in != -1)
                    write_socket_packet(socket_in, buf, rdLen);
            }
            if(socket_out != -1)
            {
                if (mode == ModeTCP)
                    do_check_modem_hangup(&socket_out, buf, rdLen);
                if(socket_out != -1)
                    write_socket_packet(socket_out, buf, rdLen);
            }
            else if (mode == ModeTCP && socket_in == -1)
                do_modem_emulation(buf, rdLen);
        }
        else if (rdLen < 0)
            misc_print(1, "ERROR: from read: %d: %s\n", rdLen, strerror(errno));
    } while (TRUE);

    close_fd();
    return 0;
}