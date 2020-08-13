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
#include <libgen.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include "serial.h"
#include "serial2.h"
#include "misc.h"
#include "udpsock.h"
#include "tcpsock.h"
#include "alsa.h"
#include "ini.h"
#include "modem.h"
#include "config.h"

enum MODE {ModeUSBMIDI, ModeTCP, ModeUDP, ModeUSBSER, ModeMUNT, ModeMUNTGM, ModeFSYNTH, ModeUDPMUNT, ModeUDPMUNTGM, ModeUDPFSYNTH};

static enum MODE        mode                   = ModeUSBMIDI;
static int              fdSerialUSB            = -1;
static int              fdMidi                 = -1;
static int              fdMidiIN               = -1;

int                     MIDI_DEBUG             = TRUE;
int                     socket_in              = -1;
int                     socket_out             = -1;
int                     socket_lst             = -1;
int                     fdSerial               = -1;
int                     baudRate               = -1;
int                     muntVolume             = -1;
int                     fsynthVolume           = -1;
int                     midilinkPriority       =  0;
int                     UDPBaudRate            = -1;
int                     TCPBaudRate            = -1;
int                     UDPBaudRate_alt        = -1;
int                     TCPBaudRate_alt        = -1;
int                     MIDIBaudRate           = -1;
int                     USBSerBaudRate         = -1;
int                     TCPFlow                = -1;
int                     TCPDTR                 =  1;
int                     UDPFlow                = -1;
int                     MUNTCPUMask            = -1;
int                     FSYNTHCPUMask          = -1;
unsigned int            DELAYSYSEX             = FALSE;
unsigned int            UDPServerPort          = 1999;
unsigned int            TCPServerPort          = 23;
unsigned int            UDPServerFilterIP      = FALSE;
char                    MT32LCDMsg[21]         = "MiSTer MidiLink! BB7";
char                    fsynthSoundFont [150]  = "/media/fat/linux/soundfonts/SC-55.sf2";
char                    MUNTRomPath[150]       = "/media/fat/linux/mt32-rom-data";
char                    UDPServer [100]        = "";
char                    mixerControl[20]       = "Master";
char                    MUNTOptions[30]        = "";
char                    USBSerModule[100]      = "";

static pthread_t        midiInThread;
static pthread_t        midiINInThread;
static pthread_t        socketInThread;
static pthread_t        socketLstThread;
static pthread_t        serialInThread;

///////////////////////////////////////////////////////////////////////////////////////
//
// void set_pcm_valume(int volume)
void set_pcm_volume(int value)
{
    if(value != -1)
    {
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
    char buf[200];
    int midiPort = -1;
    set_pcm_volume(muntVolume);
    if(strlen(MUNTOptions) > misc_count_str_chr(MUNTOptions, ' '))
        misc_print(0, "Starting --> mt32d : Options --> '%s' ", MUNTOptions);
    else
        misc_print(0, "Starting --> mt32d");
    if (CPUMASK != MUNTCPUMask)
            misc_print(0, " : CPUMASK = %d", MUNTCPUMask);
    misc_print(0, "\n");
    sprintf(buf, "taskset %d mt32d %s -f %s &", MUNTCPUMask, MUNTOptions, MUNTRomPath);
    system(buf);
    int loop = 0;
    do
    {
        if (loop > 0)
            misc_print(0, "Looking for MUNT port (%d / 2)\n", loop);
        sleep(2);
        midiPort = alsa_get_midi_port("MT-32");
        loop++;
    } while (midiPort < 0 && loop < 3);
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
    misc_print(0, "Starting --> fluidsynth");
    if (CPUMASK != FSYNTHCPUMask)
            misc_print(0, " : CPUMASK = %d", FSYNTHCPUMask);
    misc_print(0, "\n"); 
    sprintf(buf, "taskset %d fluidsynth -is -a alsa -m alsa_seq %s &", FSYNTHCPUMask, fsynthSoundFont);
    system(buf);
    int loop = 0;
    do
    {
        if(loop > 0)
            misc_print(0, "Looking for FluidSynth port (%d / 13)\n", loop);
        sleep(3);
        midiPort = alsa_get_midi_port("FLUID Synth");
        loop++;
    } while (midiPort < 0 && loop < 14);
    return midiPort;
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
            //  misc_print(2, " %02x '%c'", *byte, *byte);
            misc_print(2, " %02x", *byte);
        misc_print(2, "\n");
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
    show_debug_buf("SEQU OUT ", buf, bufLen);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// write_midi_packet()
// this is for /dev/midi
//
void write_midi_packet(char * buf, int bufLen)
{
    static int SYSEX = FALSE;
    if (DELAYSYSEX && (SYSEX || memchr(buf, 0xF0, bufLen))) // This is for MT-32 Rev0 -
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
                while(!write(fdMidi, byte, 1))
                {
                    misc_print(0, "ERROR : write_midi_packet() --> result = 0\n");
                }
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
// write_socket_packet()
// this is for TCP/IP
//
void write_socket_packet(int sock, char * buf, int bufLen)
{
    if (mode == ModeTCP)
    {
        if(tcpsock_write(sock, buf, bufLen) < 1)
        {
            close(socket_out);
            //tcpsock_close(socket_out);
            socket_out = -1;
        }
    }
    else
        udpsock_write(sock, buf, bufLen);

    show_debug_buf("SOCK OUT ", buf, bufLen);
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
// void * udpsock_thread_function_ext(void * x)
// Thread function for UDP input for external device like Raspberry Pi
//
void * udpsock_thread_function_ext (void * x)
{
    unsigned char buf[100];
    int rdLen;
    do
    {
        rdLen = udpsock_read(socket_in,  (char *) buf, sizeof(buf));
        if (rdLen > 0)
        {
            write_alsa_packet(buf, rdLen);
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
// void * serial_thread_function(void * x)
// Thread function for /dev/ttyUSB input
//
void * serial_thread_function (void * x)
{
    unsigned char buf [100];
    int rdLen;
    do
    {
        rdLen = read(fdSerialUSB, &buf, sizeof(buf));
        if (rdLen > 0)
        {
            write(fdSerial, buf, rdLen);
            show_debug_buf("SERIAL IN  ", buf, rdLen);
        }
        else
        {
            misc_print(1, "ERROR: serial_thread_function() reading %s --> %d : %s \n", serialDeviceUSB, rdLen, strerror(errno));
        }
    } while (TRUE);
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
    if (fdSerial   > 0) close (fdSerialUSB);
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
    modem_killall_mpg123(0);
    modem_killall_aplaymidi(0);
    modem_killall_aplay(0);
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
    int altBaud = FALSE;
    char coreName[30] = "";
    MUNTCPUMask = CPUMASK;
    FSYNTHCPUMask = CPUMASK;
    
    unsigned char buf[256];
    //catch_signal(SIGTERM);
    misc_print(0, "\e[2J\e[H");
    misc_print(0, helloStr);
    misc_print(0, "\n");
	
    modem_set_defaults();
    misc_get_core_name(coreName, sizeof(coreName));
    misc_print(0, "CORE --> '%s'\n", coreName);

    if(misc_check_file(midiLinkINI))
        ini_read_ini(midiLinkINI, coreName, 0);
    else 
    {
        midiLinkINI = basename(midiLinkINI);
        if(misc_check_file(midiLinkINI))
            ini_read_ini(midiLinkINI, coreName, 0);
    }

    if (misc_check_args_option(argc, argv, "QUIET"))
        MIDI_DEBUG = FALSE;
    else
        MIDI_DEBUG = TRUE;

    if (midilinkPriority != 0)
        misc_set_priority(midilinkPriority);

    if (!misc_check_device(midiDevice) && misc_check_args_option(argc, argv, "MENU"))
    {
        if (misc_check_file("/tmp/ML_MUNT"))                mode = ModeMUNT;
        if (misc_check_file("/tmp/ML_MUNTGM"))              mode = ModeMUNTGM;
        if (misc_check_file("/tmp/ML_FSYNTH"))              mode = ModeFSYNTH;
        if (misc_check_file("/tmp/ML_UDP"))               { mode = ModeUDP; altBaud = FALSE; }
        if (misc_check_file("/tmp/ML_TCP"))               { mode = ModeTCP; altBaud = FALSE; }
        if (misc_check_file("/tmp/ML_UDP_ALT"))           { mode = ModeUDP; altBaud = TRUE;  }
        if (misc_check_file("/tmp/ML_TCP_ALT"))           { mode = ModeTCP; altBaud = TRUE;  }
        if (misc_check_file("/tmp/ML_USBMIDI"))             mode = ModeUSBMIDI;
        if (misc_check_file("/tmp/ML_USBSER"))              mode = ModeUSBSER;
        if (mode == ModeUSBMIDI) // no USB MIDI 
        {
            misc_print(0, "MENU --> FSYNTH\n");
            mode = ModeFSYNTH;
        }
    }
    else
    {
        if(misc_check_args_option(argc, argv, "MUNT"))      mode = ModeMUNT;
        if(misc_check_args_option(argc, argv, "MUNTGM"))    mode = ModeMUNTGM;
        if(misc_check_args_option(argc, argv, "FSYNTH"))    mode = ModeFSYNTH;
        if(misc_check_args_option(argc, argv, "UDP"))     { mode = ModeUDP; altBaud = FALSE; }
        if(misc_check_args_option(argc, argv, "TCP"))     { mode = ModeTCP; altBaud = FALSE; }
        if(misc_check_args_option(argc, argv, "UDPALT"))  { mode = ModeUDP; altBaud = TRUE;  }
        if(misc_check_args_option(argc, argv, "TCPALT"))  { mode = ModeTCP; altBaud = TRUE;  }
        if(misc_check_args_option(argc, argv, "USBMIDI"))   mode = ModeUSBMIDI;
        if(misc_check_args_option(argc, argv, "USBSER"))    mode = ModeUSBSER;
        if(misc_check_args_option(argc, argv, "UDPMUNT"))   mode = ModeUDPMUNT;
        if(misc_check_args_option(argc, argv, "UDPMUNTGM")) mode = ModeUDPMUNTGM;
        if(misc_check_args_option(argc, argv, "UDPFSYNTH")) mode = ModeUDPFSYNTH;
    }

    modem_killall_mpg123(0);
    modem_killall_aplaymidi(0);
    killall_softsynth(3);

    if (mode == ModeMUNT || mode == ModeMUNTGM || mode == ModeFSYNTH || 
        mode == ModeUDPMUNT || mode == ModeUDPMUNTGM || mode == ModeUDPFSYNTH)
    {
        /* Disable this so we can run UDPMUNT & UDPFSYNTH on RPi or other linux device. 
        if(!misc_check_device(MrAudioDevice)) // && misc_check_file("/etc/asound.conf"))
        {
            misc_print(0, "ERROR: You have no MrAudio device in kernel --> %s\n", MrAudioDevice);
            close_fd();
            return -1;
        }
        */
        
        if (!misc_check_device(PCMDevice))
        {
            misc_print(0, "ERROR: You have no PCM device --> %s\n", PCMDevice);
            close_fd();
            return -2;
        }
        
        switch(mode)
        {
            case ModeMUNT:
            case ModeMUNTGM:
            case ModeUDPMUNT:
            case ModeUDPMUNTGM:
                midiPort = start_munt();
                break;
            case ModeFSYNTH:
            case ModeUDPFSYNTH:
                midiPort = start_fsynth();
                break;
        }

        if (midiPort < 0)
        {
            misc_print(0, "ERROR: Unable to find Synth MIDI port after several attempts :(\n");
            close_fd();
            return -3;
        }
    }

    //these modes don't need serial port. all others do :)
    if (mode != ModeUDPMUNT && mode != ModeUDPMUNTGM && mode != ModeUDPFSYNTH ) 
    {
        fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
        if (fdSerial < 0)
        {
            misc_print(0, "ERROR: opening %s: %s\n", serialDevice, strerror(errno));
            close_fd();
            return -4;
        }
        serial_set_interface_attribs(fdSerial);

        int result = misc_check_args_option(argc, argv, "BAUD");
        if(result && (result + 1 < argc))
        {
            char * ptr;
            baudRate = strtol(argv[result + 1], &ptr, 10);
            if (!serial2_is_valid_rate (baudRate))
            {
                misc_print(0, "ERROR : BAUD not valid --> %s\n", argv[result + 1]);
                return -5;
            }
        }
        else if (altBaud && mode == ModeUDP && UDPBaudRate_alt != -1)
        {
            baudRate = UDPBaudRate_alt;
        }
        else if (mode == ModeUDP && UDPBaudRate != -1)
        {
            baudRate = UDPBaudRate;
        }
        else if (mode == ModeTCP)
        {
            if(altBaud && TCPBaudRate_alt != -1)
                baudRate = TCPBaudRate_alt;
            else if(TCPBaudRate != -1)
                baudRate = TCPBaudRate;
            else
                baudRate = 115200;
        }
        else if (mode == ModeUSBSER)
        {
            if(USBSerBaudRate != -1)
                baudRate = USBSerBaudRate;
            else
                baudRate = 115200;
        }
        else
        {
            if(MIDIBaudRate != -1)
                baudRate = MIDIBaudRate;
            else
            {
                if (strcmp(coreName, "AO486") == 0)
                    baudRate = 38400;
                else
                    baudRate = 31250;
            }
        }

        serial_set_flow_control(fdSerial, 0);
        serial2_set_baud(serialDevice, fdSerial, baudRate);
        serial_do_tcdrain(fdSerial);
        serial2_set_DCD(serialDevice, fdSerial, (mode == ModeTCP)?FALSE:TRUE);
    }
    
    if (mode == ModeMUNT || mode == ModeMUNTGM || mode == ModeFSYNTH)
    {
        if(alsa_open_seq(midiPort, (mode == ModeMUNTGM)?1:0))
        {
            show_line();
            if(strlen(MT32LCDMsg) > 0)
                write_alsa_packet(buf, misc_MT32_LCD(MT32LCDMsg, buf));
            //This loop handles softSynth MUNT/FluidSynth
            misc_print(0, "Starting --> local MIDI loop :)\n");
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
            return -6;
        }
        close_fd();
        return 0;
    }

    switch(mode)
    {
    case ModeUDPMUNT:
    case ModeUDPMUNTGM:
    case ModeUDPFSYNTH:
        if(alsa_open_seq(midiPort, (mode == ModeUDPMUNTGM)?1:0))
        {
            show_line();
            if(strlen(MT32LCDMsg) > 0)
                write_alsa_packet(buf, misc_MT32_LCD(MT32LCDMsg, buf));
            socket_in  = udpsock_server_open(UDPServerPort);
            if(socket_in > 0)
            {
                misc_print(0, "Socket Listener created on port %d.\n", UDPServerPort);
                status = pthread_create(&socketInThread, NULL, udpsock_thread_function_ext, NULL);       
                if (status == -1)
                {
                    misc_print(0, "ERROR: unable to create UDP socket input thread.\n");
                    close_fd();
                    return -8;
                }
                misc_print(0, "UDP Socket input thread(2) created.\n");
            }
            else
            {
                misc_print(0, "ERROR: unable to create UDP listener --> '%s'\n", strerror(errno));
                close_fd();
                return -9;
            }
        }
        else
        {
            close_fd();
            return -10;
        }
        break;
    case ModeUDP:
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
                        misc_print(0, "ERROR: unable to create UDP socket input thread.\n");
                        close_fd();
                        return -11;
                    }
                    misc_print(0, "UDP Socket input thread(1) created.\n");
                }
                else
                {
                    misc_print(0, "ERROR: unable to create UDP listener --> '%s'\n", strerror(errno));
                    close_fd();
                    return -12;
                }
            }
            else
            {
                misc_print(0, "ERROR: unable to open UDP port --> '%s'\n", strerror(errno));
                close_fd();
                return -13;
            }
        }
        else
        {
            misc_print(0, "ERROR: in INI File (UDP_SERVER) --> %s\n", midiLinkINI);
            close_fd();
            return -14;
        }
    }
    break;
    case ModeTCP:
    {
        if(TCPFlow > 0)
            serial_set_flow_control(fdSerial, TCPFlow);
        //serial2_set_DCD(sericlDevice, fdSerial, FALSE);
        serial_set_timeout(fdSerial, 1);
        socket_lst = tcpsock_server_open(TCPServerPort);
        if(socket_lst != -1)
        {
            status = pthread_create(&socketLstThread, NULL, modem_tcplst_thread_function, NULL);
            if (status == -1)
            {
                misc_print(0, "ERROR: unable to create socket listener thread.\n");
                close_fd();
                return -15;
            }
            misc_print(0, "Socket listener thread created.\n");
        }
        else
        {
            misc_print(0, "ERROR: unable to create socket listener --> '%s'\n", strerror(errno));
            close_fd();
            return -16;
        }
    }
    break;
    case ModeUSBMIDI:
    {
        fdMidi = open(midiDevice, O_RDWR);
        if (fdMidi < 0)
        {
            misc_print(0, "ERROR: cannot open %s: %s\n", midiDevice, strerror(errno));
            close_fd();
            return -17;
        }

        if (misc_check_device(midiINDevice))
        {
            fdMidiIN = open(midiINDevice, O_RDONLY);
            if (fdMidiIN < 0)
            {
                misc_print(0, "ERROR: cannot open %s: %s\n", midiINDevice, strerror(errno));
                close_fd();
                return -18;
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
                return -19;
            }
            misc_print(0, "MIDI1 input thread created.\n");
            misc_print(0, "CONNECT : %s --> %s & %s\n", midiINDevice, serialDevice, midiDevice);
        }

        status = pthread_create(&midiInThread, NULL, midi_thread_function, NULL);
        if (status == -1)
        {
            misc_print(0, "ERROR: unable to create MIDI input thread.\n");
            close_fd();
            return -20;
        }
        misc_print(0, "MIDI input thread created.\n");
        misc_print(0, "CONNECT : %s <--> %s\n", serialDevice, midiDevice);
    }
    break;
    case ModeUSBSER:
    {
        if (!misc_check_device(serialDeviceUSB))
        {
            if(strlen(USBSerModule) > 0)
            {
                misc_print(0, "WARNING: You have no '%s' device - loading --> %s\n", serialDeviceUSB, USBSerModule);
                sprintf(buf, "insmod %s", USBSerModule);
            }
            else
            {
                misc_print(0, "ERROR: You have no '%s' device! --> maybe set 'USB_SERIAL_MODULE = ' in '%s'?\n", serialDeviceUSB, midiLinkINI);
                close_fd();
                return -21;
            }
            system(buf);
        }

        fdSerialUSB = open(serialDeviceUSB, O_RDWR);
        if (fdSerialUSB < 0)
        {
            misc_print(0, "ERROR: cannot open %s: %s\n", serialDeviceUSB, strerror(errno));
            close_fd();
            return -22;
        }

        serial_set_flow_control(fdSerial, 3);    //RTS/CTS
        serial_set_flow_control(fdSerialUSB, 3);
        serial_set_interface_attribs(fdSerialUSB);
        serial2_set_baud(serialDeviceUSB, fdSerialUSB, baudRate);
        serial_do_tcdrain(fdSerialUSB);
        serial2_set_DCD(serialDeviceUSB, fdSerialUSB, TRUE);

        status = pthread_create(&serialInThread, NULL, serial_thread_function, NULL);
        if (status == -1)
        {
            misc_print(0, "ERROR: unable to create serial input thread.\n");
            close_fd();
            return -23;
        }
        misc_print(0, "MIDI input thread created.\n");
        misc_print(0, "CONNECT : %s <--> %s\n", serialDevice, serialDeviceUSB);
    }
    break;
    }

    show_line();

    switch(mode)
    {
    case ModeUSBMIDI:
        misc_print(1, "Sending MIDI --> all-notes-off\n");
        write_midi_packet(all_notes_off, sizeof_all_notes_off);
        if(strlen(MT32LCDMsg) > 0)
        {
            misc_print(1, "Sending MT-32 LCD --> '%s'\n", MT32LCDMsg);
            write_midi_packet(buf, misc_MT32_LCD(MT32LCDMsg, buf));
        }
        //This main loop handles USB MIDI
        misc_print(0, "Starting --> USB MIDI loop :)\n");
        do
        {
            int rdLen = read(fdSerial, buf, sizeof(buf));
            if (rdLen > 0)
                write_midi_packet(buf, rdLen);
            else if (rdLen < 0)
                misc_print(1, "ERROR: (USBMIDI) from read: %d: %s\n", rdLen, strerror(errno));
        } while (TRUE);
        break;
    case ModeUSBSER:
        //This main loop handles USB serial
        misc_print(0, "Starting --> USB Serial loop :)\n");
        do
        {
            int rdLen = read(fdSerial, buf, sizeof(buf));
            if (rdLen > 0)
            {
                show_debug_buf("SERIAL OUT ", buf, rdLen);
                write(fdSerialUSB, buf, rdLen);
            }
            else if (rdLen < 0)
                misc_print(1, "ERROR: (USBSER) from read: %d: %s\n", rdLen, strerror(errno));
        } while (TRUE);
        break;
    case ModeUDPMUNT:
    case ModeUDPMUNTGM:
    case ModeUDPFSYNTH:
        misc_print(0, "Starting --> UDP Synth loop :)\n");
        do
        {
            sleep(1);
        } while (TRUE);
        break;
    case ModeUDP:
        //only send all-notes-off if UDP is being used with MIDI and not game
        if (socket_out != -1 && baudRate == 31250)
        {
            misc_print(1, "Sending UDP --> all-notes-off\n");
            write_socket_packet(socket_out, all_notes_off, sizeof_all_notes_off);
            if(strlen(MT32LCDMsg) > 0)
            {
                misc_print(1, "Sending UDP MT-32 LCD --> '%s'\n", MT32LCDMsg);
                write_socket_packet(socket_out, buf, misc_MT32_LCD(MT32LCDMsg, buf));
            }
        }
        //This main loop handles UDP
        misc_print(0, "Starting --> UDP loop :)\n");
        do
        {
            int rdLen = read(fdSerial, buf, sizeof(buf));
            if (rdLen > 0)
            {
                if(socket_out != -1)
                    write_socket_packet(socket_out, buf, rdLen);
            }
            else if (rdLen < 0)
                misc_print(1, "ERROR: (UDP) from read: %d: %s\n", rdLen, strerror(errno));
        } while (TRUE);
        break;
    case ModeTCP :
        //This main loop handles TCP
        misc_print(0, "Starting --> TCP loop :)\n");
        do
        {
            int rdLen = read(fdSerial, buf, sizeof(buf));
            if (rdLen > 0)
            {
                if(socket_in != -1)
                {
                    modem_do_check_hangup(&socket_in, buf, rdLen);
                    if(socket_in != -1)
                        write_socket_packet(socket_in, buf, rdLen);
                }
                if(socket_out != -1)
                {
                    modem_do_check_hangup(&socket_out, buf, rdLen);
                    if(socket_out != -1)
                        write_socket_packet(socket_out, buf, rdLen);
                }
                else if (socket_in == -1)
                    modem_do_emulation(buf, rdLen);
            }
            else if (TCPDTR == 2 && rdLen == 0 &&
                     serial2_get_DSR(fdSerial) == FALSE)
            {   // deal with client hangup via DTR
                if(socket_out != -1)
                {
                    tcpsock_close(socket_out);
                    socket_out = -1;
                }
                if (socket_in != -1)
                {
                    close(socket_in);
                    socket_in = -1;
                }
            }
            else if (rdLen < 0)
                misc_print(1, "ERROR: (TCP) from read: %d: %s\n", rdLen, strerror(errno));
        } while (TRUE);
        break;
    }
    close_fd();
    return 0;
}
