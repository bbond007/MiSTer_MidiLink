#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <netdb.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include "misc.h"
#include "ini.h"

///////////////////////////////////////////////////////////////////////////////////////
//
//
//
static char            pauseStr[] = "[PAUSE]";
//static char            pauseDel[sizeof(pauseStr)];
static char            pauseSpc[sizeof(pauseStr)];

static pthread_mutex_t print_lock;
static pthread_mutex_t swrite_lock;
extern int             MIDI_DEBUG;
extern enum ASCIITRANS TCPAsciiTrans;

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
//  void misc_print(int priority, const char* format, ... )
//
void misc_print(int priority, const char* format, ... )
{
    if(MIDI_DEBUG || priority == 0)
    {
        pthread_mutex_lock(&print_lock);
        va_list args;
        va_start (args, format);
        vprintf (format, args);
        va_end (args);
        pthread_mutex_unlock(&print_lock);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_str_to_upper(char *str)
//
void misc_str_to_upper(char *str)
{
    char * temp = str;
    while (*temp != '\0')
    {
        *temp = toupper(*temp);
        temp++;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  void misc_ascii_to_petsky(char * buf, int bufLen)
//
void misc_ascii_to_petskii(char * buf, int bufLen)
{
    for(int i = 0; i < bufLen; i++)
    {
        switch(buf[i])
        {
        case 0x08:
            buf[i] == 0x14;
            break;
        default:
            if (toupper(buf[i]) == buf[i])
                buf[i] = tolower(buf[i]);
            else
                buf[i] = toupper(buf[i]);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  int misc_str_to_trans(char * str)
//
int misc_str_to_trans(char * str)
{
    misc_str_to_upper(str);
    if      (strcmp(str, "PETSKII") == 0)
        return AsciiToPetskii;
    else if (strcmp(str, "ATASCII") == 0)
        return AsciiToAtascii;
    else
        return AsciiNoTrans;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  char * misc_trans_to_str(enum ASCIITRANS mode)
//
char * misc_trans_to_str(enum ASCIITRANS mode)
{
    switch(mode)
    {
    case AsciiNoTrans:
        return "NONE";
    case AsciiToPetskii:
        return "PETSKII";
    case AsciiToAtascii:
        return "ATASKII";
    default:
        return "UNKNOWN!";
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  void misc_swrite(int priority, const char* format, ... )
//
void misc_swrite(int fdSerial, const char* format, ... )
{
    char buf[512];
    pthread_mutex_lock(&swrite_lock);
    va_list args;
    va_start (args, format);
    vsprintf (buf, format, args);
    va_end (args);
    int bufLen = strlen(buf);
    switch(TCPAsciiTrans)
    {
    case AsciiToPetskii:
        misc_ascii_to_petskii(buf, bufLen);
        break;
    default:
        break;
    }
    write(fdSerial, buf, bufLen);
    pthread_mutex_unlock(&swrite_lock);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  void misc_swrite_no_trans(int priority, const char* format, ... )
//  PETSKII translation seems to screw uo Quantup-link because its looking for "connect" vs "CONNECT"
//

void misc_swrite_no_trans(int fdSerial, const char* format, ... )
{
    char buf[512];
    pthread_mutex_lock(&swrite_lock);
    va_list args;
    va_start (args, format);
    vsprintf (buf, format, args);
    va_end (args);
    write(fdSerial, buf, strlen(buf));
    pthread_mutex_unlock(&swrite_lock);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL misc_check_args_option (int argc, char *argv[], char * option)
//
int misc_check_args_option (int argc, char *argv[], char * option)
{
    int result = FALSE;
    char * OPTION = strdup(option);
    misc_str_to_upper(OPTION);
    if(argc > 1)
        for (int i = 1; i< argc; i++)
        {
            if (strcasecmp(argv[i], option) == 0)
            {
                result = i;
                break;
            }
        }
    free(OPTION);
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL misc_check_device (char * deviceName)
//
int misc_check_device (char * deviceName)
{
    struct stat filestat;
    misc_print(0, "Checking for --> '%s' : ", deviceName);
    if (stat(deviceName, &filestat) != 0)
    {
        misc_print(0, "FALSE\n");
        return FALSE;
    }
    misc_print(0, "TRUE\n");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL misc_check_device (char * fileName)
//
int misc_check_file(char * fileName)
{
    return misc_check_device(fileName);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_set_priority(int priority)
//
int misc_set_priority(int priority)
{
    misc_print(0, "Setting task priority --> %d\n", priority);
    if(setpriority(PRIO_PROCESS, getpid(), priority) == 0)
        return TRUE;
    else
        misc_print(0, "ERROR: unable to set task priority --> %s\n", strerror(errno));
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// bool misc_ipaddr_is_multicast(char * ipAddr)
//
int misc_ipaddr_is_multicast(char * ipAddr)
{
    if (strlen(ipAddr) > 3)
    {
        char * strPort, * endPtr;
        char ip1s[] = {ipAddr[0], ipAddr[1], ipAddr[2], 0x00};
        int ip1 = strtol(ip1s, &endPtr, 10);
        if(ip1 >= 224 && ip1 <= 239)
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_is_ip_addr(char *ipAddr)
//
int misc_is_ip_addr(char *ipAddr)
{
    char validChr[] = ".0987654321";
    while (*ipAddr != '\0')
        if (strchr(validChr, *ipAddr++) == NULL)
            return FALSE;
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_is_number(char *testStr)
//
int misc_is_number(char *testStr)
{
    char validChr[] = "0987654321";
    int bNum = FALSE;
    while (*testStr != '\0')
        if (strchr(validChr, *testStr++) == NULL)
            return FALSE;
        else
            bNum = TRUE;
    return bNum;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_hostname_to_ip(char * hostname , char* ip)
//
int misc_hostname_to_ip(char * hostname, char* ipAddr)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        misc_print(0, "ERROR : misc_hostname_to_ip()\n");
        return FALSE;
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ipAddr, inet_ntoa(*addr_list[i]) );
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_get_ipaddr(char * interface, char * buf)
//
int misc_get_ipaddr(char * interface, char * buf)
{
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    int result = ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    /* display result */
    sprintf(buf, "%s --> %s", interface, (result == 0)?
            inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr):"N/A");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_show_atdt(int fdSerial)
//
void misc_show_atdt(int fdSerial)
{
    char * sep = "\r\n----------------------------------";
    misc_swrite(fdSerial, "\r\nSyntax Error");
    misc_swrite(fdSerial, sep);
    misc_swrite(fdSerial, "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG");
    misc_swrite(fdSerial, "\r\nEXAMPLE --> ATDT192.168.1.100:1999");
    misc_swrite(fdSerial, "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG*1999");
    misc_swrite(fdSerial, sep);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_show_atip(int fdSerial)
//
void misc_show_atip(int fdSerial)
{
    char tmp[50];
    char * sep = "\r\n-------------------------";
    misc_swrite(fdSerial, sep);
    misc_get_ipaddr("eth0", tmp);
    misc_swrite(fdSerial, "\r\n %s", tmp);
    misc_get_ipaddr("wlan0", tmp);
    misc_swrite(fdSerial, "\r\n%s", tmp);
    misc_swrite(fdSerial, sep);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// long misc_get_time_diff(struct timeval * start, struct timeval * stop)
//
long misc_get_timeval_diff(struct timeval * start, struct timeval * stop)
{
    long secs  = stop->tv_sec  - start->tv_sec;
    long usecs = stop->tv_usec - start->tv_usec;
    return ((secs) * 1000 + usecs/1000.0);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_check_module_loaded (char * modName)
//
int misc_check_module_loaded (char * modName)
{
    char str[1024];
    FILE * file;
    char fileName[] = "/proc/modules";
    misc_print(0, "Checking kernel module loaded --> %s : ", modName);
    file = fopen(fileName, "r");
    if (file)
    {
        while (fgets(str, sizeof(str), file)!= NULL)
        {
            if (strstr(str, modName))
            {
                fclose(file);
                misc_print(0, "TRUE\n");
                return TRUE;
            }
        }
        fclose(file);
        misc_print(0, "FALSE\n");
        return FALSE;
    }
    else
    {
        misc_print(0, "ERROR: misc_check_module_loaded() : Unable to open --> '%s'\n", fileName);
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_get_midi_port(char * descr)
int misc_get_midi_port(char * descr)
{
    char str[80];
    FILE * pipe;
    char * endPtr;
    char * tmp;
    int iPort = -1;
    pipe = popen("aconnect -o", "r");
    if (pipe)
    {
        while (fgets(str, sizeof(str), pipe)!= NULL)
        {
            if(strstr(str, descr) && strlen(str) > 10)
            {
                tmp = strchr(str, ':');
                if (tmp)
                {
                    *tmp = (char) 0x00;
                    tmp = strchr(str, ' ');
                    if (tmp)
                    {
                        tmp++;
                        iPort = strtol(tmp, &endPtr, 10);
                        fclose(pipe);
                        sprintf(tmp, "echo %d > /tmp/DEBUG_ML_PORT", iPort);
                        system(tmp);
                        return iPort;
                    }
                }
            }
        }
        fclose(pipe);
        return iPort;
    }
    else
    {
        misc_print(0, "ERROR: misc_get_midi_port('%s') : Unable to open --> '%s'\n", descr, "aconnect");
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_do_pipe(int fdSerial, char * command, char * arg)
//
int misc_do_pipe(int fdSerial,  char * path, char * command,
                 char * arg1,
                 char * arg2,
                 char * arg3,
                 char * arg4,
                 char * arg5)
{
    int pipefd[2];
    if(pipe (pipefd) != -1)
    {
        if (fork() == 0)
        {
            // child
            close(pipefd[0]);                // close reading end in the child
            dup2(pipefd[1], fileno(stdout)); // fdSerial);
            dup2(pipefd[1], fileno(stderr)); // fdSerial);
            close(pipefd[1]);
            dup2(fdSerial, fileno(stdin));   // change stdin to fdSerial
            if (execl(path, command, arg1, arg2, arg3, arg4, arg5, NULL))
                misc_print(0, "ERROR: misc_do_pipe() exec failed --> %s\n", strerror(errno));
            abort();
        }
        else
        {
            // parent
            char rdBuf[1024];
            int  rdLen;
            close(pipefd[1]);                // close the write end of the
            do                               // pipe in the parent
            {
                rdLen = read(pipefd[0], rdBuf, sizeof(rdBuf));
                if(rdLen > 0)
                    write(fdSerial, rdBuf, rdLen);
            } while(rdLen > 0);
            return TRUE;
        }
    }
    else
    {
        misc_print(0, "ERROR: misc_do_pipe(') pipe --> '%s'\n", command, strerror(errno));
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_do_pipe2(int fdSerial, char * command)
//
int misc_do_pipe2(int fdSerial,  char * command)
{
    char str[1024];
    FILE * pipe;
    pipe = popen(command, "r");
    int fdStderr;
    if (pipe)
    {
        dup2(fileno(stderr), fdStderr);
        dup2(fdSerial, fileno(stderr));
        while (fgets(str, sizeof(str), pipe)!= NULL)
        {
            write(fdSerial, str, sizeof(str));
            write(fdSerial, "\r\n", 2);
        }
        fclose(pipe);
        sleep(2);
        dup2(fdStderr, fdStderr);
        return TRUE;
    }
    else
    {
        misc_print(0, "ERROR: misc_do_pipe2('%s') --> '%s'\n", command, strerror(errno));
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_count_str_char(char * str, chr chr);
//
int misc_count_str_chr(char * str, char chr)
{
    int result = 0;
    char * temp = str;
    while (*temp != '\0')
    {
        if(*temp == chr)
            result++;
        temp++;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_d_type_to_str(unsigned char type, char * buf)
//
void misc_d_type_to_str(unsigned char type, char * buf)
{
    switch(type)
    {
    case DT_BLK:      //This is a block device.
        strcpy(buf, "BLK");
        break;
    case DT_CHR:      //This is a character device.
        strcpy(buf, "CHR");
        break;
    case DT_DIR:      //This is a directory.
        strcpy(buf, "DIR");
        break;
    case DT_FIFO:     //This is a named pipe (FIFO).
        strcpy(buf, "FIF");
        break;
    case DT_LNK:      //This is a symbolic link.
        strcpy(buf, "LNK");
        break;
    case DT_REG:      //This is a regular file.
        strcpy(buf, "REG");
        break;
    case DT_SOCK:     //This is a UNIX domain socket.
        strcpy(buf, "SOK");
        break;
    case DT_UNKNOWN:  //The file type could not be determined.
        strcpy(buf, "???");
        break;
    default:
        strcpy(buf, "???");
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_list_files(char * path, int fdSerial, int rows, char * fileName, int * DIR)
//
int misc_list_files(char * path, int fdSerial, int rows, char * fileName, int * DIR)
{
    struct dirent **namelist;
    int  max;
    int  index        = 0;
    int  count        = 0;
    int  page         = 0;
    int  skip         = 0;
    char * endPtr;
    char strIdx[8];
    unsigned char c;
    char prompt[10]   = "";
    char strRows[10]  = "";
    char clrScr[]     = "\e[2J\e[H";
    char promptEnd[]  = "END  #? --> ";
    char promptMore[] = "MORE #? --> ";
    char strType[4]   = "";
    int  result       = FALSE;

    fileName[0] = (char) 0x00;
    sprintf(strRows, "%d", rows);
    char * path2   = malloc(strlen(path) + 3);
    sprintf(path2, "%s/.", path);
    max = scandir(path2, &namelist, NULL, alphasort);
    if (max < 0)
    {
        misc_swrite(fdSerial, "\r\nBad Path --> ");
        misc_swrite(fdSerial, path);
    }
    else
    {
        misc_swrite(fdSerial, clrScr);
        while (index < max)
        {
            if(strlen(namelist[index]->d_name) > 0 &&
                    namelist[index]->d_name[0] != '.')
            {
                misc_swrite(fdSerial, "%2d", count + 1);
                if (namelist[index]->d_type != DT_REG)
                {
                    misc_d_type_to_str(namelist[index]->d_type, strType);
                    misc_swrite(fdSerial, " <%s> ", strType);
                }
                else
                    misc_swrite(fdSerial, "  -->  ");
                misc_swrite(fdSerial, namelist[index]->d_name);
                misc_swrite(fdSerial, "\r\n");
                count++;
            }
            else
                skip++;
            index++;
            if ((count == rows && rows > 0) || index == max)
            {
                if(index == max)
                    misc_swrite(fdSerial, promptEnd);
                else
                    misc_swrite(fdSerial, promptMore);
                prompt[0] = (char) 0x00;
                do
                {
                    while (read(fdSerial, &c, 1) == 0) {};
                    c = toupper(c);
                    switch(c)
                    {
                    case 0x08: // [DELETE]
                    case 0x14: // [PETSKII DELETE]
                    case 0xf8: // [BACKSPACE]
                        if (strlen(prompt) > 0)
                        {
                            prompt[strlen(prompt) -1] = (char) 0x00;
                            write(fdSerial, &c, 1);
                        }
                        break;
                    case '-':
                        sprintf(fileName, "..");
                        result = TRUE;
                        *DIR = TRUE;
                        c = 'Q';
                        break;
                    case 'P':
                        if (page == 0)
                            c = (char) 0x00;
                        break;
                    case 0x0d: // [RETURN]
                        if(strlen(prompt) > 0)
                        {
                            int iMenu = strtol(prompt, &endPtr, 10) + (page * rows) + skip - 1;
                            if(iMenu < max)
                            {
                                strcpy(fileName, namelist[iMenu]->d_name);
                                result = TRUE;
                                *DIR = (namelist[iMenu]->d_type == DT_DIR)?TRUE:FALSE;
                            }
                            c = 'Q';
                        }
                        break;
                    default:
                        if(isdigit(c) && strlen(prompt) < strlen(strRows))
                        {
                            write(fdSerial, &c, 1);
                            prompt[strlen(prompt)+ 1] = (char) 0x00;
                            prompt[strlen(prompt)] = c;
                        }
                        break;
                    }
                } while (c != 'Q'  &&
                         c != 0x0d &&
                         c != ' '  &&
                         c != 'P');
                if(c == 'P')
                {
                    page--;
                    index -= (rows + count);
                }
                else
                    page++;
                count = 0;
                misc_swrite(fdSerial, clrScr);
                if (c == 'Q')
                    break;
                //else if(index < max) //no clear on exit
                //   misc_swrite(fdSerial,  clrScr);
            }
        }
        for(index = 0; index < max; index++)
            free(namelist[index]);
        free(namelist);
    }
    free(path2);
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int misc_MT32_LCD(char * MT32Message, char * bufOut)
//
// format sysex message to change LCD screen
int misc_MT32_LCD(char * MT32Message, char * buf)
{
    unsigned char tmp[] = {0xF0, 0x41, 0x10, 0x16, 0x12, 0x20, 0x00, 0x00,
                           0,0,0,0,0,   //sysex character data
                           0,0,0,0,0,   // "
                           0,0,0,0,0,   // "
                           0,0,0,0,0,   // "
                           0x00, /* checksum placedholder */
                           0xF7  /* end of sysex */
                          };
    unsigned char checksum = 0;
    int MT32messageIndex = 0;
    for (int tmpIndex = 5; tmpIndex < sizeof(tmp) - 2; tmpIndex++)
    {
        if (tmpIndex > 7)
        {
            if (MT32messageIndex < strlen(MT32Message))
                tmp[tmpIndex] = MT32Message[MT32messageIndex++];
            else
                tmp[tmpIndex] = 0x20;
        }
        checksum += tmp[tmpIndex];
    }
    checksum = 128 - checksum % 128;
    tmp[sizeof(tmp) - 2] = checksum;
    memcpy(buf, tmp, sizeof(tmp));
    return sizeof(tmp);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_do_rowcheck(int fdSerial, int rows, int * rowcount, char * c)
//
void misc_do_rowcheck(int fdSerial, int rows, int * rowcount, char * c)
{
    (*rowcount)++;
    if (*rowcount == rows)
    {
        //if(pauseDel[0] != 0x08)
            //memset(pauseDel, 0x08, sizeof(pauseDel));
        if(pauseSpc[0] != ' ')
            memset(pauseSpc, ' ', sizeof(pauseSpc));
        misc_swrite(fdSerial, "\r\n");
        misc_swrite(fdSerial, pauseStr);
        while (read(fdSerial, c, 1) == 0) {};
        //misc_swrite(fdSerial, pauseDel);
        misc_swrite(fdSerial, "\r");
        misc_swrite(fdSerial, pauseSpc);
        misc_swrite(fdSerial, "\r");
        *rowcount = 0;
        *c = toupper(*c);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_show_at_commands(int fdSerial, int rows)
//
void misc_show_at_commands(int fdSerial, int rows)
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
// BOOL misc_do_pipe2(int fdSerial, char * command)
//
int misc_file_to_serial(int fdSerial,  char * fileName, int rows)
{
    char str[1024];
    FILE * file;
    int index = 0;
    int rowcount = 0;
    char c = (char) 0x00;
    file = fopen(fileName, "r");
    if (file)
    {
        //misc_swrite(fdSerial, "\r\n");
        while (fgets(str, sizeof(str), file) != NULL && c != 'Q')
        {
            misc_swrite(fdSerial, "\r");
            if(rowcount != 0 || index == 0)
                misc_swrite(fdSerial, "\n");                
            misc_replace_char(str, strlen(str), '\n', 0x00);
            misc_replace_char(str, strlen(str), '\r', 0x00);
            misc_swrite(fdSerial, str);
            index++;
            misc_do_rowcheck(fdSerial, rows, &rowcount, &c);
        }
        fclose(file);
        return TRUE;
    }
    else
    {
        misc_print(0, "ERROR: misc_file_to_serial('%s') --> '%s'\n", fileName, strerror(errno));
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char misc_replace_char(char * str, int strLen, char old, char new)
//
char misc_replace_char(char * str, int strLen, char old, char new)
{
    for(int i = 0; i < strLen; i++)
        if(str[i] == old)
            str[i] = new;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// misc_get_core_name(char * buf, int maxBuf)
//

int misc_get_core_name(char * buf, int maxBuf)
{
    FILE * file;
    char * fileName = "/tmp/CORENAME";
    buf[0] = 0x00;
    file = fopen(fileName, "r");
    if (file)
    {
        fgets(buf, maxBuf, file);
        fclose(file);
        misc_replace_char(buf, strlen(buf), 0x0a, 0x00);
        misc_replace_char(buf, strlen(buf), 0x0d, 0x00);
        misc_str_to_upper(buf);
        return TRUE;
    }
    else
    {
        misc_print(0, "ERROR: misc_get_core_name() : Unable to open --> '%s'\n", fileName);
        return FALSE;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
// char * misc_hayes_flow_to_str(int flow) 
//
char * misc_hayes_flow_to_str(int flow)
{
    switch(flow)
    {
        case 0: return "Diasble Flow-control";
        case 3: return "RTS/CTS";
        case 4: return "XON/XOFF";
        default: 
            return "UNKNOWN";
    }
    
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char * misc_hayes_DTR_to_str(int dtr) 
//
char * misc_hayes_DTR_to_str(int dtr)
{
    switch(dtr)
    {
        case 1: return "Normal";
        case 2: return "Hangup";
        default: 
            return "UNKNOWN";
    }
    
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char * misc_hayes_ATQ_to_str(int dtr) 
//
char * misc_hayes_ATQ_to_str(int dtr)
{
    switch(dtr)
    {
        case 0: return "Normal";
        case 1: return "Quite - no result codes";
        default: 
            return "UNKNOWN";
    }
    
}
