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

///////////////////////////////////////////////////////////////////////////////////////
//
//  misc_print( const char* format, ... )
//
static pthread_mutex_t print_lock;
extern int MIDI_DEBUG;
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
// void_misc__str_to_upper(char *str)
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
            char * arg = strdup(argv[i]);
            misc_str_to_upper(arg);
            if (strcmp(arg, option) == 0)
                result = TRUE;
            free(arg);
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
    misc_print(0, "Checking for --> %s : ", deviceName);
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

    for(int i = 0; i < strlen(ipAddr); i++)
        if (strchr(validChr, ipAddr[i])!= NULL)
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
    for(int i = 0; i < strlen(testStr); i++)
        if (strchr(validChr, testStr[i]) == NULL)
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
// void misc_write_ok6(int fdSerial)
//
void misc_write_ok6(int fdSerial)
{
    write(fdSerial, "\r\nOK\r\n", 6);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_write_ok4(int fdSerial)
//
void misc_write_ok4(int fdSerial)
{
    write(fdSerial, "OK\r\n", 4);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_show_atdt(int fdSerial)
//
void misc_show_atdt(int fdSerial)
{
    char serror   [] = "\r\nSyntax Error";
    char line[] = "\r\n----------------------------------";
    char example1 [] = "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG";
    char example2 [] = "\r\nEXAMPLE --> ATDT192.168.1.100:1999";
    char example3 [] = "\r\nEXAMPLE --> ATDTBBS.DOMAIN.ORG*1999";
    write(fdSerial, serror,   strlen(serror));
    write(fdSerial, line,     strlen(line));
    write(fdSerial, example1, strlen(example1));
    write(fdSerial, example2, strlen(example2));
    write(fdSerial, example3, strlen(example3));
    misc_write_ok6(fdSerial);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void misc_show_atip(int fdSerial)
//
void misc_show_atip(int fdSerial)
{
    char tmp[50];
    sprintf(tmp, "\r\n-------------------------\r\n");
    write(fdSerial, tmp, strlen(tmp));
    misc_get_ipaddr("eth0", tmp);
    write(fdSerial, " ", 1);
    write(fdSerial, tmp, strlen(tmp));
    write(fdSerial, "\r\n", 2);
    misc_get_ipaddr("wlan0", tmp);
    write(fdSerial, tmp, strlen(tmp));
    misc_write_ok6(fdSerial);
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
int misc_do_pipe(int fdSerial,  char * command, char * arg)
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
            if (execl(command, command, arg, (char *) NULL) < 0)
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
// int misc_do_pipe2(int fdSerial, char * command)
//
int misc_file_to_serial(int fdSerial,  char * fileName)
{
    char str[1014];
    FILE * file;
    file = fopen(fileName, "r");
    if (file)
    {
        write(fdSerial, "\r\n", 2);
        while (fgets(str, sizeof(str), file)!= NULL)
        {
            write(fdSerial, str, strlen(str));
            write(fdSerial, "\r", 1);
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
// int misc_count_str_char(char * str, chr chr);
//
int misc_count_str_chr(char * str, char chr)
{
   int result = 0;
   while(*str != (char) 0x00)
       if(*str++ == chr) result++;
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
    char c;
    char prompt[10]   = "";
    char strRows[10]  = "";
    int  result       = FALSE;
    char clrScr[]     = "\e[2J\e[H";
    char promptEnd[]  = "END  #? --> ";
    char promptMore[] = "MORE #? --> ";
    char strType[4]   = "";

    fileName[0] = (char) 0x00;
    sprintf(strRows, "%d", rows);
    char * path2   = malloc(strlen(path) + 3);
    sprintf(path2, "%s/.", path);
    max = scandir(path2, &namelist, NULL, alphasort);
    if (max < 0)
    {
        char err[] = "\r\nBad Path --> ";
        write(fdSerial, err, strlen(err));
        write(fdSerial, path, strlen(path));
    }
    else
    {
        //write(fdSerial, "\r\n", 2);
        write(fdSerial, clrScr, strlen(clrScr));
        while (index < max)
        {
            if(strlen(namelist[index]->d_name) > 0 &&
                    namelist[index]->d_name[0] != '.')
            {
                sprintf(strIdx, "%6d", count + 1);
                write(fdSerial,strIdx, strlen(strIdx));
                if (namelist[index]->d_type != DT_REG)
                {
                    misc_d_type_to_str(namelist[index]->d_type, strType);
                    write (fdSerial, " <", 2);
                    write (fdSerial, strType, strlen(strType));
                    write (fdSerial, "> ", 2);
                }
                else
                    write (fdSerial, "  -->  ", 7); 
                write(fdSerial, namelist[index]->d_name, strlen(namelist[index]->d_name));
                write(fdSerial, "\r\n", 2);
                count++;
            }
            else
                skip++;
            index++;
            if ((count == rows && rows > 0) || index == max)
            {
                if(index == max)
                    write (fdSerial, promptEnd,  strlen(promptEnd));
                else
                    write (fdSerial, promptMore, strlen(promptMore));
                prompt[0] = (char) 0x00;
                do
                {
                    read(fdSerial, &c, 1);
                    c = toupper(c);
                    switch(c)
                    {
                    case 0x08: // [DELETE]
                    case 0xf8: // [BACKSPACE]
                        if (strlen(prompt) > 0)
                        {
                            prompt[strlen(prompt) -1] = (char) 0x00;
                            write(fdSerial, &c, 1);
                        }
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
                write(fdSerial, clrScr, strlen(clrScr));
                if (c == 'Q')
                    break;
                //else if(index < max) //no clear on exit
                //   write(fdSerial, clrScr, strlen(clrScr));
            }
        }
        for(index = 0; index < max; index++)
            free(namelist[index]);
        free(namelist);
    }
    free(path2);
    return result;
}