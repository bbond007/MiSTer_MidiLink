/*--------------------------------------------------------------------
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <linux/soundcard.h>
#include <ctype.h>
#include <sys/time.h>
#include "setbaud.h"
#include "serial.h"
#include "config.h"
#include "misc.h"
#include "udpsock.h"
#include "tcpsock.h"
#include "alsa.h"
#include "ini.h"
#include "directory.h"

enum MODE {ModeNULL, ModeTCP, ModeUDP, ModeMUNT, ModeMUNTGM, ModeFSYNTH};

int                     MIDI_DEBUG	       = TRUE;
static enum MODE        mode                   = ModeNULL;
static int		fdSerial	       = -1;
static int		fdMidi		       = -1;
static int		fdMidiIN	       = -1;
static int 		socket_in	       = -1;
static int 		socket_out	       = -1;
static int 		socket_lst             = -1;
static int 		baudRate	       = -1;
char                    MT32LCDMsg[21]         = "MiSTer MidiLink! BB7";
char 		        MP3Path[500]           = "/media/fat/MP3";
char 		        MIDIPath[500]          = "/media/fat/MIDI";
char 		        downloadPath[500]      = "/media/fat";
char                    uploadPath[100]        = "/media/fat/UPLOAD";
char         		fsynthSoundFont [150]  = "/media/fat/soundfonts/SC-55.sf2";
char         		UDPServer [100]        = "";
char 			mixerControl[20]       = "Master";
char 			MUNTOptions[30]        = "";
int                     MP3Volume              = -1;
int 			muntVolume             = -1;
int 			fsynthVolume           = -1;
int 			midilinkPriority       = 0;
int                     UDPBaudRate            = -1;
int                     TCPBaudRate            = -1;
int                     TCPFlow                = -1;
int                     UDPFlow                = -1;
enum SOFTSYNTH          TCPSoftSynth           = FluidSynth;
unsigned int 		TCPTermRows            = 22;
unsigned int            DELAYSYSEX	       = FALSE;
unsigned int 		UDPServerPort          = 1999;
unsigned int 		TCPServerPort          = 23;
unsigned int            UDPServerFilterIP      = FALSE;
static pthread_t	midiInThread;
static pthread_t	midiINInThread;
static pthread_t	socketInThread;
static pthread_t        socketLstThread;

///////////////////////////////////////////////////////////////////////////////////////
//
// void set_pcm_valume(int volume)
void set_pcm_volume(int value)
{
    if(value != -1)
    {
        //if(misc_check_module_loaded("snd_dummy"))
        //    strcpy(mixerControl, "Master");
        char buf[30];
        sprintf(buf, "amixer set %s %d%c",     mixerControl, value, '%');
        misc_print(0, "Setting '%s' to %d%\n", mixerControl, value);
        system(buf);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void killall_softsynth(int delay)
//
void killall_softsynth(int delay)
{
    misc_print(0, "Killing --> fluidsynth\n");
    system("killall -q fluidsynth");
    misc_print(0, "Killing --> mt32d\n");
    system("killall -q mt32d");
    if(delay)
        sleep(delay);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int start_munt()
//
int start_munt()
{
    char buf[60];
    int midiPort = -1;
    set_pcm_volume(muntVolume);
    if(strlen(MUNTOptions) > misc_count_str_chr(MUNTOptions, ' '))
        misc_print(0, "Starting --> mt32d : Options --> '%s'\n", MUNTOptions);
    else
        misc_print(0, "Starting --> mt32d\n");
    sprintf(buf, "taskset %d mt32d %s &", CPUMASK, MUNTOptions);
    system(buf);
    int loop = 0;
    do
    {
        sleep(2);
        midiPort = alsa_get_midi_port("MT-32");
        loop++;
    }
    while(midiPort < 0 && loop < 3);
    return midiPort;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int start_fsynth()
//
int start_fsynth()
{
    char buf[256];
    int midiPort = -1;
    set_pcm_volume(fsynthVolume);
    misc_print(0, "Starting --> fluidsynth\n");
    sprintf(buf, "taskset %d fluidsynth -is -a alsa -m alsa_seq %s &", CPUMASK, fsynthSoundFont);
    system(buf);
    int loop = 0;
    do
    {
        sleep(2);
        midiPort = alsa_get_midi_port("FLUID Synth");
        loop++;
    }
    while(midiPort < 0 && loop < 5);
    return midiPort;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void killall_mpg123()
//
void killall_mpg123(int delay)
{
    misc_print(0, "Killing --> mpg123\n");
    system("killall -q mpg123");
    if(delay)
        sleep(delay);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void killall_aplaymidi()
//
void killall_aplaymidi(int delay)
{
    misc_print(0, "Killing --> aplaymidi\n");
    system("killall -q aplaymidi");
    if(delay)
        sleep(delay);
}
///////////////////////////////////////////////////////////////////////////////////////
//
// void show_debug_buf(char * descr, char * buf, int bufLen)
//
void show_debug_buf(char * descr, char * buf, int bufLen)
{
    static struct timeval start = {0, 0};
    struct timeval time;
    if(MIDI_DEBUG)
    {
        if(start.tv_sec == 0)
            gettimeofday(&start, NULL);
        gettimeofday(&time, NULL);
        misc_print(2, "[%08ld] %s[%02d] -->", misc_get_timeval_diff (&start, &time), descr, bufLen);
        for (unsigned char * byte = buf; bufLen-- > 0; byte++)
            misc_print(2, " %02x ", *byte);
        misc_print(2, "\n");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * tcplst_thread_function(void * x)
// Thread function for TCP Listener input
//
void * tcplst_thread_function (void * x)
{
    unsigned char buf[100];
    int rdLen;

    do
    {
        socket_in = tcpsock_accept(socket_lst);
        if(socket_in != -1)
        {
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
        }
        else
        {
            sleep(5);
        }

    } while(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * socket_thread_function(void * x)
// Thread function for TCP input
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
    pthread_exit(NULL);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * udpsock_thread_function(void * x)
// Thread function for UDP input
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
    static struct timeval start;
    static struct timeval stop;
    char tmp[100] = "";

    for (char * p = buf; bufLen-- > 0; p++)
    {
        switch(*p)
        {
        case 0x0d:// [RETURN]
            if(memcmp(lineBuf, "+++ATH", 6) == 0)
            {
                int delay = misc_get_timeval_diff(&start, &stop);
                if(delay > 900)
                {
                    tcpsock_close(*socket);
                    *socket =  -1;
                    sprintf(tmp, "\r\nHANG-UP DETECTED\r\n");
                    misc_print(1, "HANG-UP Detected --> %d\n", delay);
                    write(fdSerial, tmp, strlen(tmp));
                    sleep(1);
                    write(fdSerial, "OK\r\n", 4);
                }
                else
                    misc_print(1, "HANG-UP Rejected --> %d\n", delay);
            }
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
            lastChar = 0x0d;
            break;
        case '+': // RESET BUFFER
            gettimeofday(&start, NULL);
            if (lastChar != '+')
                iLineBuf = 0;
        default:
            if (lastChar == '+' && *p != '+')
                gettimeofday(&stop, NULL);
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
// void do_telnet_negotiate()
//
//
#define DO   0xfd
#define WONT 0xfc
#define WILL 0xfb
#define DONT 0xfe
#define CMD  0xff
#define CMD_ECHO 1
#define CMD_WINDOW_SIZE 31

void do_telnet_negotiate()
{
    char buf[3];
    int rdLen1;
    int rdLen2;
    int rdLen3;
    int wrLen = 0;
    unsigned char msg1[] = {0xff, 0xfb, 0x1f};
    unsigned char msg2[] = {0xff, 0xfa, 0x1f, 0x00, 0x50, 0x00, 0x18, 0xff, 0xf0};

    tcpsock_set_timeout(socket_out, 3);
    misc_print(1, "Telnet negotiation --> START\n");
    do
    {
        rdLen1  = tcpsock_read(socket_out, &buf[0], 1);
        if (rdLen1 == 1 && buf[0] == CMD)
        {
            //misc_print(1, "Telnet negotiation --> CMD\n");
            rdLen2 = tcpsock_read(socket_out, &buf[1], 1);
            rdLen3 = tcpsock_read(socket_out, &buf[2], 1);
            if (buf[1] == DO && buf[2] == CMD_WINDOW_SIZE)
            {
                //misc_print(1, "Telnet negotiation --> N1\n");
                wrLen = tcpsock_write(socket_out, msg1, sizeof(msg1));
                if (wrLen < 0) goto end;
                wrLen = tcpsock_write(socket_out, msg2, sizeof(msg2));
                if (wrLen < 0) goto end;
            }
            else
            {
                //misc_print(1, "Telnet negotiation --> N2\n");
                for (int i = 0; i < sizeof(buf); i++)
                {
                    if (buf[i] == DO)
                        buf[i] = WONT;
                    else if (buf[i] == WILL)
                        buf[i] = DO;
                }
                wrLen = tcpsock_write(socket_out, buf, sizeof(buf));
                if (wrLen < 0) goto end;
                buf[0] = CMD;
            }
        }
        else if(rdLen1 = 1)
            write(fdSerial, &buf[0], 1);
    } while (buf[0] == CMD && rdLen1 == 1);
end:
    misc_print(1, "Telnet negotiation --> END\n");
    if (wrLen < 0)
    {
        misc_print(0, "ERROR: Telnet negotiation failed\n");
    }
    tcpsock_set_timeout(socket_out, 0);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL do_file_picker(char * pathBuf, char * resultBuf)
//
//
int do_file_picker(char * pathBuf, char * fileNameBuf)
{
    int DIR = 0;
    int result;
    char * endPtr;
    do
    {
        result = misc_list_files(pathBuf, fdSerial, TCPTermRows, fileNameBuf, &DIR);
        if(result)
            if (DIR)
            {
                if(strcmp(fileNameBuf, ".") == 0 ||
                        strcmp(fileNameBuf, "..") == 0)
                {
                    endPtr = strrchr(pathBuf, '/');
                    if (endPtr != NULL && strlen(pathBuf) > 1)
                        *endPtr = (char) 0x00;
                }
                else
                {
                    strcat(pathBuf, "/");
                    strcat(pathBuf, fileNameBuf);
                }
            }
            else
            {
                char msg[] = "Selected file --> ";
                write (fdSerial, msg, strlen(msg));
                write(fdSerial, fileNameBuf, strlen(fileNameBuf));
            }
    } while (result && DIR);
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int get_softsynth_port(int softSynth)
//
//
int get_softsynth_port(int softSynth)
{
    int midiPort = alsa_get_midi_port("MT-32");
    if (midiPort == -1)
        midiPort = alsa_get_midi_port("FLUID Synth");
    if (midiPort == -1)
    {
        switch(softSynth)
        {
        case MUNT:
            midiPort = start_munt();
            break;
        case FluidSynth:
            midiPort = start_fsynth();
            break;
        case -1: //do nothing.
            break;
        }
    }
    return midiPort;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void do_modem_emulation(char * buf, int bufLen)
//
//

#define KILL_MP3_SLEEP if(MP3){killall_mpg123(1);MP3 = FALSE;}

void do_modem_emulation(char * buf, int bufLen)
{
    static char lineBuf[150]    = "";
    static char iLineBuf        = 0;
    static int TELNET_NEGOTIATE = TRUE;
    static int MP3              = FALSE;
    char tmp[1024]              = "";
    char * endPtr;
    char fileName [256];
    char audioError[]           = "\r\nBad news, you have no audio device :( --> %s";

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
                char * ipAddr = &lineBuf[4];
                directory_search(midiLinkDIR, ipAddr, ipAddr);
                char * prtSep  = strchr(ipAddr, ':');
                if(prtSep == NULL)
                    prtSep = strchr(ipAddr, '*'); // with NCOMM?
                char * port   = (prtSep == NULL)?NULL:(prtSep + 1);
                if (prtSep != NULL) *prtSep = 0x00;
                if (strlen(ipAddr) < 3)
                    misc_show_atdt(fdSerial);
                else
                {
                    int ipError = FALSE;
                    int iPort = (port == NULL)?23:strtol(port, &endPtr, 10);
                    if (!misc_is_ip_addr(ipAddr))
                    {
                        char domainName[30];
                        getdomainname(domainName, sizeof(domainName));
                        if(strcmp(domainName, "(none)") != 0 && misc_count_str_chr(ipAddr, '.') < 1)
                        {
                            strcat(ipAddr, ".");
                            strcat(ipAddr, domainName);
                            misc_print(1, "Doing domain name fix --> %s\n", ipAddr);
                        }
                        if(!misc_hostname_to_ip(ipAddr, ipAddr))
                        {
                            sprintf(tmp, "\r\nERROR: Unable to convert hostname '%s' to IP address.", ipAddr);
                            write(fdSerial, tmp, strlen(tmp));
                            ipError = TRUE;
                        }
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
                        if (TELNET_NEGOTIATE)
                            do_telnet_negotiate();
                        int status = pthread_create(&socketInThread, NULL, tcpsock_thread_function, NULL);
                    }
                    else
                        misc_write_ok6(fdSerial);
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
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATIP", 4) == 0)
            {
                misc_show_atip(fdSerial);
            }
            else if (memcmp(lineBuf, "AT&K", 4) == 0)
            {
                char * hayesMode = &lineBuf[4];
                if(misc_is_number(hayesMode))
                {
                    int iHayesMode = strtol(hayesMode, &endPtr, 10);
                    serial_set_flow_control(fdSerial, iHayesMode);
                }
                else
                    serial_set_flow_control(fdSerial, -1);
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATTEL", 5) == 0)
            {
                if (lineBuf[5] == '0')
                    TELNET_NEGOTIATE = FALSE;
                else if (lineBuf[5] == '1')
                    TELNET_NEGOTIATE = TRUE;
                sprintf(tmp, "\r\nTelnet Negotiations --> %s", TELNET_NEGOTIATE?"TRUE":"FALSE");
                write(fdSerial, tmp, strlen(tmp));
            }
            else if (memcmp(lineBuf, "ATMP3", 5) == 0)
            {
                if (misc_check_device(PCMDevice))
                {
                    if(lineBuf[5] == '!')
                    {
                        sprintf(tmp, "\r\nMP3 --> OFF");
                        write(fdSerial, tmp, strlen(tmp));
                        killall_mpg123(0);
                    }
                    else if(do_file_picker(MP3Path, fileName))
                    {
                        chdir("/root");
                        set_pcm_volume(MP3Volume);
                        sprintf(tmp, "taskset %d mpg123 -o alsa \"%s/%s\" 2> /tmp/mpg123 & ", CPUMASK, MP3Path, fileName);
                        if(!MP3)
                        {
                            killall_aplaymidi(0);
                            killall_softsynth(3);
                        }
                        killall_mpg123(0);
                        misc_print(1, "Play MP3 --> %s\n", tmp);
                        system(tmp);
                        write(fdSerial, "\r\n", 2);
                        sleep(1);
                        misc_file_to_serial(fdSerial, "/tmp/mpg123", TCPTermRows);
                        MP3 = TRUE;
                    }
                }
                else
                {
                    sprintf(tmp, audioError, PCMDevice);
                    write(fdSerial, tmp, strlen(tmp));
                }
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATMID", 5) == 0)
            {
                if (misc_check_device(PCMDevice))
                {
                    if(lineBuf[5] == '!')
                    {
                        killall_aplaymidi(0);
                        sprintf(tmp, "\r\nMIDI --> OFF");
                        write(fdSerial, tmp, strlen(tmp));
                        system(tmp);
                        sleep(2);
                        int midiPort = get_softsynth_port(-1);
                        if(midiPort != -1)
                        {
                            alsa_open_seq(midiPort, 0);
                            alsa_send_midi_raw(all_notes_off, sizeof(all_notes_off));
                            alsa_close_seq();
                        }
                    }
                    else if(lineBuf[5] == '1')
                    {
                        KILL_MP3_SLEEP;
                        killall_aplaymidi(0);
                        sprintf(tmp, "\r\nLoading --> FluidSynth");
                        write(fdSerial, tmp, strlen(tmp));
                        killall_softsynth(3);
                        TCPSoftSynth = FluidSynth;
                        get_softsynth_port(TCPSoftSynth);
                    }
                    else if(lineBuf[5] == '2')
                    {
                        KILL_MP3_SLEEP;
                        killall_aplaymidi(0);
                        sprintf(tmp, "\r\nLoading --> MUNT");
                        write(fdSerial, tmp, strlen(tmp));
                        killall_softsynth(3);
                        TCPSoftSynth = MUNT;
                        get_softsynth_port(TCPSoftSynth);
                    }
                    else if(lineBuf[5] == 'S' && lineBuf[6] == 'F')
                    {
                        strcpy(tmp, fsynthSoundFont);
                        char * dir = strrchr(tmp, '/');
                        if(dir != NULL)
                            *dir = (char) 0x00;
                        else
                            tmp[0] = (char) 0x00;
                        if (do_file_picker(tmp, fileName))
                        {
                            strcpy(fsynthSoundFont, tmp);
                            strcat(fsynthSoundFont, "/");
                            strcat(fsynthSoundFont, fileName);
                            write(fdSerial, "\r\n SoundFont -->", 16);
                            write(fdSerial, fsynthSoundFont, strlen(fsynthSoundFont));
                            sprintf(tmp,"sed -i '{s|^FSYNTH_SOUNDFONT[[:space:]]*=.*|FSYNTH_SOUNDFONT    = %s|}' %s",
                                    fsynthSoundFont, midiLinkINI);
                            system(tmp);
                        }
                    }
                    else if(do_file_picker(MIDIPath, fileName))
                    {
                        KILL_MP3_SLEEP;
                        killall_aplaymidi(0);
                        int midiPort = get_softsynth_port(TCPSoftSynth);
                        chdir("/root");
                        sprintf(tmp, "taskset %d aplaymidi --port %d \"%s/%s\" 2> /tmp/aplaymidi & ", CPUMASK, midiPort, MIDIPath, fileName);;
                        misc_print(1, "Play MIDI --> %s\n", tmp);
                        system(tmp);
                        write(fdSerial, "\r\n", 2);
                        sleep(1);
                        misc_file_to_serial(fdSerial, "/tmp/aplaymidi", 0);
                        MP3 = FALSE;
                    }
                }
                else
                {
                    sprintf(tmp, audioError, PCMDevice);
                    write(fdSerial, tmp, strlen(tmp));
                }
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATSZ", 4) == 0)
            {
                if(do_file_picker(downloadPath, fileName))
                {
                    sprintf(tmp, "%s/%s", downloadPath, fileName);
                    misc_print(1, "Zmodem download --> %s\n", tmp);
                    serial_do_tcdrain(fdSerial);
                    misc_do_pipe(fdSerial, "/bin/sz","sz", tmp, NULL, NULL, NULL, NULL);
                    sleep(3);
                }
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATRZ", 4) == 0)
            {
                if(chdir (uploadPath) == 0)
                {
                    sprintf(tmp, "\r\nUpload path --> '%s'", uploadPath);
                    write(fdSerial, tmp, strlen(tmp));
                    sprintf(tmp, "\r\nUpload file using Zmodem protocol now...\r\n");
                    write(fdSerial, tmp, strlen(tmp));
                    serial_do_tcdrain(fdSerial);
                    misc_do_pipe(fdSerial, "/bin/rz", "rz", NULL, NULL, NULL, NULL, NULL);
                    chdir("/root");
                    sleep(3);
                }
                else
                {
                    sprintf(tmp, "\r\nERROR: Upload path invalid --> '%s'", uploadPath);
                    write(fdSerial, tmp, strlen(tmp));
                }
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATROWS", 6) == 0)
            {
                char * strRows = &lineBuf[6];
                if(!misc_is_number(strRows))
                {
                    for (int i = 50; i > 0; i--)
                    {
                        sprintf(tmp, "\r\n%2d", i);
                        write(fdSerial, tmp, strlen(tmp));
                    }
                    write(fdSerial, "\r\n",2);
                }
                else
                {
                    TCPTermRows  = strtol(strRows, &endPtr, 10);
                    sprintf(tmp, "\r\nROWS --> %d", TCPTermRows);
                    write(fdSerial, tmp, strlen(tmp));
                    serial_do_tcdrain(fdSerial);
                    misc_write_ok6(fdSerial);
                }
            }
            else if (memcmp(lineBuf, "ATINI", 5) == 0)
            {
                misc_file_to_serial(fdSerial, midiLinkINI, TCPTermRows);
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATDIR", 5) == 0)
            {
                misc_file_to_serial(fdSerial, midiLinkDIR, TCPTermRows);
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATVER", 5) == 0)
            {
                write(fdSerial, "\r\n",2);
                write(fdSerial, helloStr, strlen(helloStr));
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "ATHELP", 6) == 0)
            {
                misc_show_at_commands(fdSerial, TCPTermRows);
                misc_write_ok6(fdSerial);
            }
            else if (memcmp(lineBuf, "AT", 2) == 0)
            {
                if (lineBuf[2] != (char) 0x00)
                {
                    sprintf(buf, "\r\nUNKNOWN command '%s'", &lineBuf[2]);
                    write(fdSerial, buf, strlen(buf));
                }
                misc_write_ok6(fdSerial);
            }    
            else
            {
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
    {   // spoon-feed the SYSEX data and delay after
        misc_print(2, "MIDI OUT [%02d] -->", bufLen);
        for (unsigned char * byte = buf; bufLen-- > 0; byte++)
        {
            switch (*byte)
            {
            case 0xF0: // SYSEX START
                SYSEX = TRUE;
                misc_print(2, " SXD+ f0");
                write(fdMidi, byte, 1);
                usleep(1000);
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
                if (SYSEX) usleep(1000);
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
void * midiINin_thread_function (void * x)
{
    unsigned char buf[100];             // bytes from sequencer driver
    int rdLen;
    do
    {
        rdLen = read(fdMidiIN, &buf, sizeof(buf));
        if (rdLen < 0)
        {
            misc_print(1, "ERROR: midiINin_thread_function() reading %s --> %d : %s \n", midiINDevice, rdLen, strerror(errno));
            sleep(10);
            if (misc_check_device(midiINDevice))
            {
                misc_print(1, "Reopening  %s --> %d : %s \n", midiINDevice);
                fdMidiIN = open(midiINDevice, O_RDONLY);
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
// void close_fd()
//
void close_fd()
{
    if (fdSerial   > 0) close (fdSerial);
    if (fdMidi     > 0) close (fdMidi);
    if (fdMidiIN   > 0) close (fdMidiIN);
    if (socket_in  > 0) tcpsock_close(socket_in);
    if (socket_out > 0) tcpsock_close(socket_out);
    if (socket_lst > 0) tcpsock_close(socket_lst);
    alsa_close_seq();
}

///////////////////////////////////////////////////////////////////////////////////////
//
// sig_term_handler(int signum, siginfo_t *info, void *ptr)
//
void signal_handler(int signum, siginfo_t *info, void *ptr)
{
    //write(STDERR_FILENO, SIGTERM_MSG, sizeof(SIGTERM_MSG));
    killall_mpg123(0);
    killall_aplaymidi(0);
    killall_softsynth(0);
    close_fd();
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void catch_signal(int signum)
//
void catch_signal(int signum)
{
    static struct sigaction _sigact;
    memset(&_sigact, 0, sizeof(_sigact));
    _sigact.sa_sigaction = signal_handler;
    _sigact.sa_flags = SA_SIGINFO;
    sigaction(signum, &_sigact, NULL);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{
    int status;
    int midiPort;
    unsigned char buf[256];
    //catch_signal(SIGTERM);
    misc_print(0, "\e[2J\e[H");
    misc_print(0, helloStr);
    misc_print(0, "\r");

    if(misc_check_file(midiLinkINI))
        ini_read_ini(midiLinkINI);

    if (misc_check_args_option(argc, argv, "QUIET"))
        MIDI_DEBUG = FALSE;
    else
        MIDI_DEBUG = TRUE;

    if (midilinkPriority != 0)
        misc_set_priority(midilinkPriority);

    int MIDI = misc_check_device(midiDevice);

    if (misc_check_args_option(argc, argv, "MENU") && !MIDI)
    {
        if (misc_check_file("/tmp/ML_MUNT")   && !MIDI)   mode   = ModeMUNT;
        if (misc_check_file("/tmp/ML_MUNTGM") && !MIDI)   mode   = ModeMUNTGM;
        if (misc_check_file("/tmp/ML_FSYNTH") && !MIDI)   mode   = ModeFSYNTH;
        if (misc_check_file("/tmp/ML_UDP"))               mode   = ModeUDP;
        if (misc_check_file("/tmp/ML_TCP"))               mode   = ModeTCP;
        if (mode != ModeMUNT && mode != ModeMUNTGM && mode != ModeFSYNTH &&
                mode != ModeTCP && mode != ModeUDP)
        {
            misc_print(0, "MENU --> FSYNTH\n");
            mode = ModeFSYNTH;
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

    killall_mpg123(0);
    killall_aplaymidi(0);
    killall_softsynth(3);

    if (mode == ModeMUNT || mode == ModeMUNTGM || mode == ModeFSYNTH)
    {
        if(!misc_check_device(MrAudioDevice)) // && misc_check_file("/etc/asound.conf"))
        {
            misc_print(0, "ERROR: You have no MrAudio device in kernel --> %s\n", MrAudioDevice);
            return -1;
        }

        if (!misc_check_device(PCMDevice))
        {
            //misc_print(0, "ERROR: You have no PCM device loading --> snd-dummy module\n");
            //system ("modprobe snd-dummy");
            misc_print(0, "ERROR: You have no PCM device --> %s\n", PCMDevice);
            return -2;
        }

        if (mode == ModeMUNT || mode == ModeMUNTGM)
            midiPort = start_munt();
        else if (mode == ModeFSYNTH)
            midiPort = start_fsynth();

        if (midiPort < 0)
        {
            misc_print(0, "ERROR: Unable to find Synth MIDI port after several attempts :(\n");
            close_fd();
            return -3;
        }
    }
    fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
    if (fdSerial < 0)
    {
        misc_print(0, "ERROR: opening %s: %s\n", serialDevice, strerror(errno));
        close_fd();
        return -4;
    }
    //printf("TST --> serial_set_interface_attribs - start\n");
    serial_set_interface_attribs(fdSerial);
    //printf("TST --> serial_set_interface_attribs - end\n");
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
        if(alsa_open_seq(midiPort, (mode == ModeMUNTGM)?1:0))
        {
            show_line();
            if(strlen(MT32LCDMsg) > 0)
                write_alsa_packet(buf, misc_MT32_LCD(MT32LCDMsg, buf));
            //This loop handles softSynth MUNT/FluidSynth
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
            return -4;
        }
        close_fd();
        return 0;
    }

    if (mode == ModeUDP)
    {
        if(UDPFlow > 0)
            serial_set_flow_control(fdSerial, UDPFlow);
        if (strlen(UDPServer) > 7)
        {
            if(!misc_is_ip_addr(UDPServer))
                misc_hostname_to_ip(UDPServer, UDPServer);
            misc_print(0, "Connecting to server --> %s:%d\n", UDPServer, UDPServerPort);
            socket_out = udpsock_client_connect(UDPServer, UDPServerPort);
            if(socket_out != -1)
            {
                socket_in  = udpsock_server_open(UDPServerPort);
                if(socket_in > 0)
                {
                    misc_print(0, "Socket Listener created on port %d.\n", UDPServerPort);
                    status = pthread_create(&socketInThread, NULL, udpsock_thread_function, NULL);
                    if (status == -1)
                    {
                        misc_print(0, "ERROR: unable to create socket input thread.\n");
                        close_fd();
                        return -6;
                    }
                    misc_print(0, "Socket input thread created.\n");
                }
                else
                {
                    misc_print(0, "ERROR: unable to create UDP listener --> '%s'\n", strerror(errno));
                    close_fd();
                    return -7;
                }
            }
            else
            {
                misc_print(0, "ERROR: unable to open UDP port --> '%s'\n", strerror(errno));
                close_fd();
                return -8;
            }
        }
        else
        {
            misc_print(0, "ERROR: in INI File (UDP_SERVER) --> %s\n", midiLinkINI);
            close_fd();
            return -9;
        }
    }
    else if (mode == ModeTCP)
    {
        if(TCPFlow > 0)
            serial_set_flow_control(fdSerial, TCPFlow);
        socket_lst = tcpsock_server_open(TCPServerPort);
        if(socket_lst != -1)
        {
            status = pthread_create(&socketLstThread, NULL, tcplst_thread_function, NULL);
            if (status == -1)
            {
                misc_print(0, "ERROR: unable to create socket listener thread.\n");
                close_fd();
                return -10;
            }
            misc_print(0, "Socket listener thread created.\n");
        }
        else
        {
            misc_print(0, "ERROR: unable to create socket listener --> '%s'\n", strerror(errno));
            close_fd();
            return -11;
        }
    }
    else
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            misc_print(0, "ERROR: cannot open %s: %s\n", midiDevice, strerror(errno));
            close_fd();
            return -12;
        }

        if (misc_check_device(midiINDevice))
        {
            fdMidiIN = open(midiINDevice, O_RDONLY);
            if (fdMidiIN < 0)
            {
                misc_print(0, "ERROR: cannot open %s: %s\n", midiINDevice, strerror(errno));
                close_fd();
                return -13;
            }
        }

        if (misc_check_args_option(argc, argv, "TESTMIDI")) //Play midi test note
        {
            misc_print(0, "Testing --> %s\n", midiDevice);
            test_midi_device();
            sleep(2);
        }

        if (fdMidiIN != -1)
        {
            status = pthread_create(&midiINInThread, NULL, midiINin_thread_function, NULL);
            if (status == -1)
            {
                misc_print(0, "ERROR: unable to create *MIDI input thread.\n");
                close_fd();
                return -14;
            }
            misc_print(0, "MIDI1 input thread created.\n");
            misc_print(0, "CONNECT : %s --> %s & %s\n", midiINDevice, serialDevice, midiDevice);
        }

        status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);
        if (status == -1)
        {
            misc_print(0, "ERROR: unable to create MIDI input thread.\n");
            close_fd();
            return -14;
        }
        misc_print(0, "MIDI input thread created.\n");
        misc_print(0, "CONNECT : %s <--> %s\n", serialDevice, midiDevice);
    }

    show_line();
    //send all-notes-off to real MIDI device
    if(fdMidi != -1)
    {
        misc_print(1, "Sending MIDI --> all-notes-off\n");
        write_midi_packet(all_notes_off, sizeof(all_notes_off));
        if(strlen(MT32LCDMsg) > 0)
        {
            misc_print(1, "Sending MT-32 LCD --> '%s'\n", MT32LCDMsg);
            write_midi_packet(buf, misc_MT32_LCD(MT32LCDMsg, buf));
        }
    }
    //only send all-notes-off if UDP is being used with MIDI and not game
    if (mode == ModeUDP && socket_out != -1 && baudRate == 31250)
    {
        misc_print(1, "Sending UDP --> all-notes-off\n");
        write_socket_packet(socket_out, all_notes_off, sizeof(all_notes_off));
        if(strlen(MT32LCDMsg) > 0)
        {
            misc_print(1, "Sending UDP MT-32 LCD --> '%s'\n", MT32LCDMsg);
            write_socket_packet(socket_out, buf, misc_MT32_LCD(MT32LCDMsg, buf));
        }
    }

    //This main loop handles USB MIDI, UDP & TCP
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