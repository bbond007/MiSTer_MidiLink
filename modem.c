#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include "misc.h"
#include "tcpsock.h"
#include "modem_snd.h"
#include "alsa.h"
#include "ini.h"
#include "serial.h"
#include "serial2.h"
#include "directory.h"

#define DEFAULT_MODEMSOUND     TRUE
#define DEFAULT_modemVolume   -1
#define DEFAULT_TCPAsciiTrans  AsciiNoTrans
#define DEFAULT_TCPTermRows    22
#define DEFAULT_TCPFlow       -1
#define DEFAULT_TCPDTR         1
#define DEFAULT_TCPQuiet       0

enum SOFTSYNTH          TCPSoftSynth           = FluidSynth;
enum ASCIITRANS         TCPAsciiTrans          = DEFAULT_TCPAsciiTrans;
char                    MP3Path[500]           = "/media/fat/MP3";
char                    MIDIPath[500]          = "/media/fat/MIDI";
char                    downloadPath[500]      = "/media/fat";
char                    uploadPath[100]        = "/media/fat/UPLOAD";
char                    modemConnectSndWAV[50] = "";
char                    modemDialSndWAV[50]    = "";
char                    modemRingSndWAV[50]    = "";
int                     TCPATHDelay            = 900;
int                     MP3Volume              = -1;
unsigned int            TCPTermRows            = DEFAULT_TCPTermRows;
int                     modemVolume            = DEFAULT_modemVolume;
int                     TCPQuiet               = DEFAULT_TCPQuiet;
int                     MODEMSOUND             = DEFAULT_MODEMSOUND;

extern int              MIDI_DEBUG;
extern int              CPUMASK; 
extern int              MODEMSOUND;
extern int              socket_in;
extern int              socket_out;
extern int              socket_lst;
extern int              fdSerial;
extern int              baudRate;
extern int              TCPQuiet; 
extern int              modemVolume;
extern int              sizeof_all_notes_off;
extern int              TCPDTR;
extern int              TCPFlow;
extern char             all_notes_off[];
extern char           * midiLinkINI;    
extern char 	      * PCMDevice;
extern char           * helloStr;
extern char         	fsynthSoundFont[];
extern char             serialDevice[20];
extern char             midiLinkDIR[50];

static pthread_t        socketInThread;

int  start_munt();
int  start_fsynth();
void show_debug_buf(char * descr, char * buf, int bufLen);
void killall_softsynth(int delay);
void set_pcm_volume(int value);

static char * athelp[] =
{
    "AT       - Attention",
    "ATBAUD#  - Set baud rate",
    "ATBAUD   - Show baud rate menu",
    "ATDIR    - Show dialing MidiLink.DIR",
    "ATDT     - Dial 'ATDT192.168.1.131:23'",
    "ATHELP   - Show valid AT Comamnds",
    "ATINI    - Show MidiLink.INI",
    "ATIP     - Show IP address",
    "ATMID1   - Switch synth to FluidSynth",
    "ATMID2   - Switch synth to MUNT",
    "ATMID    - Play MIDI file",
    "ATMIDSF  - Select FluidSynth SoundFont",
    "ATMID!   - Stop currently playing MIDI",
    "ATM0     - Disable modem sounds",
    "ATM1     - Enable modem sounds",
    "ATM###%%  - Set modem volume [0-100%%]",
    "ATMP3    - Play MP3 file",
    "ATMP3!   - Stop playing MP3 File",
    "ATROWS   - Do terminal row test",
    "ATROWS## - Set number of terminal rows",
    "ATRZ     - Receive a file using Zmodem",
    "ATSZ     - Send a file via Zmodem",
    "ATTEL0   - Disable telnet negotiation",
    "ATTEL1   - Enable telnet negotiation",
    "ATTRANS# - Set ASCII translation",
    "ATQ0     - Verbose result codes",
    "ATQ1     - Suppress result codes",
    "ATVER    - Show MidiLink version",
    "ATZ      - Reset modem",
    "AT&D0    - DTR mode normal",
    "AT&D2    - DTR drop causes hangup",
    "AT&K0    - Disable  flow control",
    "AT&K3    - RTS/CTS  flow control",
    "AT&K4    - XON/XOFF flow control",
    "+++ATH   - Hang-up",
    NULL
};

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_show_at_commands(int fdSerial, int rows)
//
void modem_show_at_commands(int fdSerial, int rows)
{
    int index = 0;
    int rowcount = 0;
    char c = (char) 0x00;

    while(athelp[index] != NULL && c != 'Q')
    {
        misc_swrite(fdSerial, "\r");
        if (rowcount != 0 || index == 0) //rowcount not reset
            misc_swrite(fdSerial, "\n");
        misc_swrite(fdSerial, athelp[index]);
        index++;
        misc_do_rowcheck(fdSerial, rows, &rowcount, &c);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_killall_mpg123()
//
void modem_killall_mpg123(int delay)
{
    misc_print(0, "Killing --> mpg123\n");
    system("killall -q mpg123");
    if(delay)
        sleep(delay);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_killall_aplaymidi()
//
void modem_killall_aplaymidi(int delay)
{
    misc_print(0, "Killing --> aplaymidi\n");
    system("killall -q aplaymidi");
    if(delay)
        sleep(delay);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_killall_aplay()
//
void modem_killall_aplay(int delay)
{
    misc_print(0, "Killing --> aplay\n");
    system("killall -q aplay");
    if(delay)
        sleep(delay);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int modem_get_softsynth_port(int softSynth)
//
//
int modem_get_softsynth_port(int softSynth)
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
// void modem_play_conenct_sound(char * tmp)
//
//
void modem_play_connect_sound(char * tmp)
{
    if (MODEMSOUND)
    {
        modem_killall_aplaymidi(0);
        killall_softsynth(0);
        modem_killall_mpg123(0);
        modem_killall_aplay(0);
        if(strlen(modemConnectSndWAV) > 0 && misc_check_file(modemConnectSndWAV))
        {
            misc_print(1, "Playing WAV --> '%s'\n", modemConnectSndWAV);
            sprintf(tmp, "aplay %s", modemConnectSndWAV);
            system(tmp);
        }
        else
            modem_snd("C");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void play_conenct_sound(char * tmp)
//
//
void modem_play_ring_sound(char * tmp)
{
    if (MODEMSOUND)
    {
        modem_killall_aplaymidi(0);
        killall_softsynth(0);
        modem_killall_mpg123(0);
        modem_killall_aplay(0);
        if(strlen(modemRingSndWAV) > 0 && misc_check_file(modemRingSndWAV))
        {
            misc_print(1, "Playing WAV --> '%s'\n", modemRingSndWAV);
            sprintf(tmp, "aplay %s", modemRingSndWAV);
            system(tmp);
        }
        else
            modem_snd("R");
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_play_dial_sound(char * tmp, char * ipAddr)
//
//
void modem_play_dial_sound(char * tmp, char * ipAddr)
{
    if (MODEMSOUND)
    {
        modem_killall_aplaymidi(0);
        killall_softsynth(0);
        modem_killall_mpg123(0);
        modem_killall_aplay(0);
        if (strlen(modemDialSndWAV) > 0 && misc_check_file(modemDialSndWAV))
        {
            misc_print(1, "Playing WAV --> '%s'\n", modemDialSndWAV);
            sprintf(tmp, "aplay %s", modemDialSndWAV);
            system(tmp);
        }
        else
            modem_snd(ipAddr);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * modem_socket_thread_function(void * x)
// Thread function for TCP input
//
void * modem_tcpsock_thread_function (void * x)
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
        else if(rdLen < 1)
            misc_print(1, "ERROR: tcpsock_thread_function() --> rdLen < 1\n");
    } while (rdLen > 0 && socket_out != -1);
    if(socket_out != -1)
        close(socket_out);
    socket_out = -1;
    if(MIDI_DEBUG)
        misc_print(1, "TCPSOCK Thread fuction exiting.\n", socket_out);
    if(TCPQuiet == 0)
        misc_swrite_no_trans(fdSerial, "\r\nNO CARRIER\r\n");
    pthread_exit(NULL);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void * modem_tcplst_thread_function(void * x)
// Thread function for TCP Listener input
//
void * modem_tcplst_thread_function (void * x)
{
    unsigned char buf[100];
    int rdLen;

    do
    {
        socket_in = tcpsock_accept(socket_lst);
        if(socket_in != -1)
        {
            tcpsock_set_tcp_nodelay(socket_in);
            tcpsock_set_timeout(socket_in, 10);
            misc_print(1,"Incomming connection\n");
            tcpsock_get_ip(socket_in, buf);
            misc_print(1, "CONNECT --> %s\n", buf);
            if(socket_out == -1)
            {
                if(TCPQuiet == 0)
                    misc_swrite_no_trans(fdSerial, "\r\nRING");
                if(MODEMSOUND)
                    set_pcm_volume(modemVolume);
                modem_play_ring_sound(buf);
                modem_play_connect_sound(buf);
                if(TCPQuiet == 0)
                    misc_swrite_no_trans(fdSerial, "\r\nCONNECT %d\r\n", baudRate);
                serial2_set_DCD(serialDevice, fdSerial, TRUE);
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
                        if(socket_in != -1)
                            close(socket_in);
                        socket_in = -1;
                        misc_print(1, "tcplst_thread_function() --> Connection Closed.\n");
                    }
                } while (socket_in != -1);
                if(TCPQuiet == 0)
                    misc_swrite_no_trans(fdSerial, "\r\nNO CARRIER\r\n");
                serial2_set_DCD(serialDevice, fdSerial, FALSE);
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
// void modem_set_defaults()
//
//
void modem_set_defaults()
{
    MODEMSOUND       = DEFAULT_MODEMSOUND;
    modemVolume      = DEFAULT_modemVolume;
    TCPAsciiTrans    = DEFAULT_TCPAsciiTrans;
    TCPTermRows      = DEFAULT_TCPTermRows;
    TCPFlow          = DEFAULT_TCPFlow;
    TCPDTR           = DEFAULT_TCPDTR;
    TCPQuiet         = DEFAULT_TCPQuiet;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_do_check_hangup(char * buf, int bufLen)
//
//
void modem_do_check_hangup(int * socket, char * buf, int bufLen)
{
    static char lineBuf[6];
    static char iLineBuf = 0;
    static int plusCount = 0;
    static int NEEDSTOP = FALSE;
    static struct timeval start;
    static struct timeval stop;
    char tmp[100] = "";

    for (char * p = buf; bufLen-- > 0; p++)
    {
        switch(*p)
        {
        case '+': // RESET
            gettimeofday(&start, NULL);
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
            plusCount++;
            NEEDSTOP = TRUE;
            break;
        case 0x0D:// [RETURN]
            if(plusCount >= 3 && iLineBuf >= 3 && memcmp(lineBuf, "ATH", 3) == 0)
            {
                int delay = misc_get_timeval_diff(&start, &stop);
                if(TCPATHDelay == 0 || delay > TCPATHDelay)
                {
                    tcpsock_close(*socket);
                    *socket =  -1;
                    serial2_set_DCD(serialDevice, fdSerial, FALSE);
                    sprintf(tmp, "\r\nHANG-UP DETECTED\r\n");
                    misc_print(1, "HANG-UP Detected --> %d\n", delay);
                    misc_swrite(fdSerial, tmp);
                    sleep(1);
                    if(TCPQuiet == 0)
                        misc_swrite_no_trans(fdSerial, "OK\r\n");
                }
                else
                    misc_print(1, "HANG-UP Rejected --> %d\n", delay);
            }
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
            plusCount = 0;
            break;
        default:
            if (plusCount >= 3 && iLineBuf < sizeof(lineBuf)-1)
            {
                if (NEEDSTOP)
                {
                    gettimeofday(&stop, NULL);
                    NEEDSTOP = FALSE;
                }
                lineBuf[iLineBuf++] = *p;
                lineBuf[iLineBuf] = 0x00;
            }
            else
                plusCount = 0;
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

void modem_do_telnet_negotiate()
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
                misc_swrite(fdSerial, "Selected file --> ");
                misc_swrite(fdSerial,  fileNameBuf);
            }
    } while (result && DIR);
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL modem_handle_at_command(char * lineBuf)
//
//
#define KILL_MP3_SLEEP if(MP3){modem_killall_mpg123(1);MP3 = FALSE;}
int modem_handle_at_command(char * lineBuf)
{
    static int TELNET_NEGOTIATE = TRUE;
    static int MP3              = FALSE;
    char tmp[1024]              = "";
    char * endPtr;
    char fileName [256];
    char audioError[]           = "\r\nBad news, you have no audio device :( --> %s";

    if(memcmp(lineBuf, "ATDT", 4) == 0)
    {
        char * ipAddr = &lineBuf[4];
        if(ipAddr[0] != (char) 0x00)
            directory_search(midiLinkDIR, ipAddr, ipAddr);
        char * prtSep  = strchr(ipAddr, ':');
        if(prtSep == NULL)
            prtSep = strchr(ipAddr, '*'); // with NCOMM?
        char * port = (prtSep == NULL)?NULL:(prtSep + 1);
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
                    misc_swrite(fdSerial, "\r\nERROR: Unable to convert hostname '%s' to IP address.", ipAddr);
                    ipError = TRUE;
                }
            }
            if(!ipError)
            {
                misc_swrite(fdSerial, "\r\nDIALING %s:%d\r\n", ipAddr, iPort);
                serial_do_tcdrain(fdSerial);
                if(MODEMSOUND)
                    set_pcm_volume(modemVolume);
                modem_play_dial_sound(tmp, ipAddr);
                serial_do_tcdrain(fdSerial);
                if(MODEMSOUND)
                    sleep(1);
                serial2_set_DCD(serialDevice, fdSerial, FALSE);
                socket_out = tcpsock_client_connect(ipAddr, iPort, fdSerial);
            }
            if(socket_out > 0)
            {
                if (TELNET_NEGOTIATE)
                    modem_do_telnet_negotiate();
                modem_play_ring_sound(tmp);
                modem_play_connect_sound(tmp);
                if(TCPQuiet == 0)
                    misc_swrite_no_trans(fdSerial, "\r\nCONNECT %d\r\n", baudRate);
                serial_do_tcdrain(fdSerial);
                sleep(1);
                int status = pthread_create(&socketInThread, NULL, modem_tcpsock_thread_function, NULL);
                serial2_set_DCD(serialDevice, fdSerial, TRUE);
                return TRUE;
            }
        }
    }
    else if (memcmp(lineBuf, "ATBAUD", 6) == 0)
    {
        char * baud = &lineBuf[6];
        int iBaud   = strtol(baud, &endPtr, 10);
        int iTemp   = serial2_baud_at_index(iBaud);
        iBaud = (misc_is_number(baud) && iTemp > 0)?iTemp:iBaud;
        if (serial2_is_valid_rate (iBaud))
        {
            int sec = 10;
            misc_swrite(fdSerial, "\r\nSetting BAUD to %d in %d seconds...", iBaud, sec);
            sleep(sec);
            serial2_set_baud(serialDevice, fdSerial, iBaud);
            baudRate = iBaud;
            misc_swrite(fdSerial, "\r\nBAUD has been set to %d", iBaud);
        }
        else
        {
            if(baud[0] != 0x00)
                misc_swrite(fdSerial, "\r\nBAUD rate '%s' is not valid.", baud);
            serial2_show_menu(fdSerial);
        }
    }
    else if (memcmp(lineBuf, "ATIP", 4) == 0)
    {
        misc_show_atip(fdSerial);
    }
    else if (memcmp(lineBuf, "ATK", 3) == 0)
    {
        char * hayesMode = &lineBuf[3];
        if(misc_is_number(hayesMode))
        {
            int iHayesMode = strtol(hayesMode, &endPtr, 10);
            serial_set_flow_control(fdSerial, iHayesMode);
        }
        else
            serial_set_flow_control(fdSerial, -1);
    }
    else if (memcmp(lineBuf, "ATTEL", 5) == 0)
    {
        if (lineBuf[5] == '0')
            TELNET_NEGOTIATE = FALSE;
        else if (lineBuf[5] == '1')
            TELNET_NEGOTIATE = TRUE;
        misc_swrite(fdSerial, "\r\nTelnet Negotiations --> %s", TELNET_NEGOTIATE?"TRUE":"FALSE");
    }
    else if (memcmp(lineBuf, "ATMP3", 5) == 0)
    {
        if (misc_check_device(PCMDevice))
        {
            if(lineBuf[5] == '!')
            {
                misc_swrite(fdSerial, "\r\nMP3 --> OFF");
                modem_killall_mpg123(0);
            }
            else if(do_file_picker(MP3Path, fileName))
            {
                chdir("/root");
                set_pcm_volume(MP3Volume);
                sprintf(tmp, "taskset %d mpg123 -o alsa \"%s/%s\" 2> /tmp/mpg123 & ", CPUMASK, MP3Path, fileName);
                if(!MP3)
                {
                    modem_killall_aplaymidi(0);
                    killall_softsynth(3);
                }
                modem_killall_mpg123(0);
                misc_print(1, "Play MP3 --> %s\n", tmp);
                system(tmp);
                misc_swrite(fdSerial, "\r\n");
                sleep(1);
                misc_file_to_serial(fdSerial, "/tmp/mpg123", TCPTermRows);
                MP3 = TRUE;
            }
        }
        else
            misc_swrite(fdSerial, audioError, PCMDevice);
    }
    else if (memcmp(lineBuf, "ATMID", 5) == 0)
    {
        if (misc_check_device(PCMDevice))
        {
            if(lineBuf[5] == '!')
            {
                modem_killall_aplaymidi(0);
                misc_swrite(fdSerial, "\r\nMIDI --> OFF");
                sleep(2);
                int midiPort = modem_get_softsynth_port(-1);
                if(midiPort != -1)
                {
                    alsa_open_seq(midiPort, 0);
                    alsa_send_midi_raw(all_notes_off, sizeof_all_notes_off);
                    alsa_close_seq();
                }
            }
            else if(lineBuf[5] == '1')
            {
                KILL_MP3_SLEEP;
                modem_killall_aplaymidi(0);
                misc_swrite(fdSerial, "\r\nLoading --> FluidSynth");
                killall_softsynth(3);
                TCPSoftSynth = FluidSynth;
                modem_get_softsynth_port(TCPSoftSynth);
            }
            else if(lineBuf[5] == '2')
            {
                KILL_MP3_SLEEP;
                modem_killall_aplaymidi(0);
                misc_swrite(fdSerial, "\r\nLoading --> MUNT");
                killall_softsynth(3);
                TCPSoftSynth = MUNT;
                modem_get_softsynth_port(TCPSoftSynth);
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
                    misc_swrite(fdSerial, "\r\n SoundFont -->");
                    misc_swrite(fdSerial, fsynthSoundFont);
                    sprintf(tmp,"sed -i '{s|^FSYNTH_SOUNDFONT[[:space:]]*=.*|FSYNTH_SOUNDFONT    = %s|}' %s",
                            fsynthSoundFont, midiLinkINI);
                    system(tmp);
                }
            }
            else if(do_file_picker(MIDIPath, fileName))
            {
                KILL_MP3_SLEEP;
                modem_killall_aplaymidi(0);
                int midiPort = modem_get_softsynth_port(TCPSoftSynth);
                chdir("/root");
                sprintf(tmp, "taskset %d aplaymidi --port %d \"%s/%s\" 2> /tmp/aplaymidi & ", CPUMASK, midiPort, MIDIPath, fileName);;
                misc_print(1, "Play MIDI --> %s\n", tmp);
                system(tmp);
                misc_swrite(fdSerial, "\r\n");
                sleep(1);
                misc_file_to_serial(fdSerial, "/tmp/aplaymidi", 0);
                MP3 = FALSE;
            }
        }
        else
        {
            misc_swrite(fdSerial, audioError, PCMDevice);
        }
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
    }
    else if (memcmp(lineBuf, "ATRZ", 4) == 0)
    {
        if(chdir (uploadPath) == 0)
        {
            misc_swrite(fdSerial, "\r\nUpload path --> '%s'", uploadPath);
            misc_swrite(fdSerial, "\r\nUpload file using Zmodem protocol now...\r\n");
            serial_do_tcdrain(fdSerial);
            misc_do_pipe(fdSerial, "/bin/rz", "rz", NULL, NULL, NULL, NULL, NULL);
            chdir("/root");
            sleep(3);
        }
        else
            misc_swrite(fdSerial, "\r\nERROR: Upload path invalid --> '%s'", uploadPath);
    }
    else if (memcmp(lineBuf, "ATROWS", 6) == 0)
    {
        char * strRows = &lineBuf[6];
        if(!misc_is_number(strRows))
        {
            for (int i = 50; i > 1; i--)
                misc_swrite(fdSerial, "\r\n%2d", i);
        }
        else
        {
            TCPTermRows  = strtol(strRows, &endPtr, 10);
            misc_swrite(fdSerial, "\r\nROWS --> %d", TCPTermRows);
            serial_do_tcdrain(fdSerial);
        }
    }
    else if (memcmp(lineBuf, "ATINI", 5) == 0)
    {
        misc_file_to_serial(fdSerial, midiLinkINI, TCPTermRows);
    }
    else if (memcmp(lineBuf, "ATDIR", 5) == 0)
    {
        misc_file_to_serial(fdSerial, midiLinkDIR, TCPTermRows);
    }
    else if (memcmp(lineBuf, "ATTRANS", 7) == 0)
    {
        char * validOptions = "\r\nValid options --> 0, 1, 2"
                              "\r\n-------------------------"
                              "\r\n0 = NONE"
                              "\r\n1 = PETSKII (C64)"
                              "\r\n2 = ATASCII (Atari 8-bit)"
                              "\r\n-------------------------";
        if(misc_is_number(&lineBuf[7]))
        {
            int tmpMode  = strtol(&lineBuf[7], &endPtr, 10);
            if (tmpMode >= 0 && tmpMode <= 2)
                TCPAsciiTrans = tmpMode;
            else
                misc_swrite(fdSerial, validOptions);
        }
        else if(lineBuf[7] != (char) 0x00)
            misc_swrite(fdSerial, validOptions);
        misc_swrite(fdSerial, "\r\nASCII translation --> %s",
                    misc_trans_to_str(TCPAsciiTrans));
    }
    else if (memcmp(lineBuf, "ATM", 3) == 0)
    {
        char * pct = strchr(&lineBuf[3], '%');
        if (pct != NULL)
            *pct = (char) 0x00;
        if(misc_is_number(&lineBuf[3]))
        {
            int tmpVol  = strtol(&lineBuf[3], &endPtr, 10);
            if(pct)
            {
                if(tmpVol <= 100)
                {
                    modemVolume = tmpVol;
                }
                else
                {
                    misc_swrite(fdSerial,"\r\nValid options --> 0-100%%");
                }
            }
            else
                switch(tmpVol)
                {
                case 0:
                    MODEMSOUND = FALSE;
                    break;
                case 1:
                    MODEMSOUND = TRUE;
                    break;
                default:
                    misc_swrite(fdSerial,"\r\nUnsupported option --> '%s'", &lineBuf[3]);
                    break;
                }
        }
        else
        {
            if(lineBuf[3] != (char) 0x00)
            {
                misc_swrite(fdSerial,"\r\nUnsupported option --> '%s'", &lineBuf[3]);
            }
        }
        if(modemVolume != -1 && MODEMSOUND)
            misc_swrite(fdSerial, "\r\nModem sounds = %s : volume = %d%%", MODEMSOUND?"ON":"OFF", modemVolume);
        else
            misc_swrite(fdSerial,"\r\nModem sounds = %s", MODEMSOUND?"ON":"OFF");
    }
    else if (memcmp(lineBuf, "ATVER", 5) == 0)
    {
        misc_swrite(fdSerial, "\r\n");
        misc_swrite(fdSerial, helloStr);
    }
    else if (memcmp(lineBuf, "ATHELP", 6) == 0)
    {
        modem_show_at_commands(fdSerial, TCPTermRows);
    }
    else if (memcmp(lineBuf, "ATUARTTEST", 6) == 0)
    {
        if (lineBuf[10] == '!')
            TCPTermRows  = 0;
        while (TRUE)
        {
            modem_show_at_commands(fdSerial, TCPTermRows);
            misc_file_to_serial(fdSerial, midiLinkDIR, TCPTermRows);
        }
    }
    else if (memcmp(lineBuf, "ATD", 3) == 0)
    {
        switch(lineBuf[3])
        {
        case '0' :
        case '1' :
            TCPDTR = 1;
            misc_print(1, "Setting DTR mode --> Normal\n");
            break;
        case '2' :
            TCPDTR = 2;
            misc_print(1, "Setting DTR mode --> Hangup\n");
            break;
        default:
            misc_swrite(fdSerial,"\r\nUnsupported DTR option --> '%s'", &lineBuf[3]);
            break;
        }
    }
    else if (memcmp(lineBuf, "ATQ", 3) == 0)
    {
        switch(lineBuf[3])
        {
        case '0' :
            TCPQuiet = 0;
            misc_print(1, "Setting result code mode --> Verbose\n");
            break;
        case '1' :
            TCPQuiet = 1;
            misc_print(1, "Setting result code mode --> Quiet\n");
            break;
        default:
            misc_swrite(fdSerial,"\r\nUnsupported result code mode --> '%s'", &lineBuf[3]);
            break;
        }
    }
    else if (memcmp(lineBuf, "ATZ", 3) == 0)
    {
        misc_print(1, "Resetting TCP defaults...\n");
        modem_set_defaults();
        TELNET_NEGOTIATE = TRUE;
        misc_print(1, "Reloading INI defaults...\n");
        misc_get_core_name(tmp, sizeof(tmp));
        if(misc_check_file(midiLinkINI))
            ini_read_ini(midiLinkINI, tmp, 1);
    }
    else if (memcmp(lineBuf, "AT", 2) == 0)
    {
        if (lineBuf[2] != (char) 0x00)
        {
            misc_swrite(fdSerial, "\r\nUnknown Command '%s'", &lineBuf[2]);
            misc_print(1, "ERROR : Unknown AT command --> '%s'\n", &lineBuf[2]);
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void modem_do_emulation(char * buf, int bufLen)
//
//
void modem_do_emulation(char * buf, int bufLen)
{
    static char lineBuf[150]    = "";
    static char iLineBuf        = 0;
    char * lbp;

    show_debug_buf("SER OUT  ", buf, bufLen);
    for (char * p = buf; bufLen-- > 0; p++)
    {
        *p = toupper(*p);
        switch(*p)
        {
        case ' ':          // [SPACE] no space
            break;
        case 0x08:         // [DELETE]
        case 0x14:         // [PETSKII DELETE]
        case 0xf8:         // [BACKSPACE]
            if (iLineBuf > 0)
            {
                lineBuf[--iLineBuf] = 0x00;
                write(fdSerial, p, 1);
            }
            break;
        case 0x0D:         // [RETURN]
            lbp = lineBuf;
            int CONNECT = FALSE;
            if(iLineBuf > 1 && lineBuf[0] == 'A' && lineBuf[1] == 'T')
            {
                while (lbp && !CONNECT)
                {
                    char * amp = strchr(lbp, '&');
                    if(amp)
                        *amp = 0x00;
                    CONNECT = modem_handle_at_command(lbp);
                    if(amp)
                    {
                        lbp = amp-1;
                        lbp[0] = 'A';
                        lbp[1] = 'T';
                    }
                    else
                        lbp = NULL;
                }
                if (!CONNECT && TCPQuiet == 0)
                    misc_swrite_no_trans(fdSerial, "\r\nOK\r\n");
            }
            else
                misc_swrite(fdSerial, "\r\n");
            iLineBuf = 0;
            lineBuf[iLineBuf] = 0x00;
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

