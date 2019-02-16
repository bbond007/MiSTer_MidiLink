#include <stdlib.h>
#include <string.h>
#include <asm/termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include"misc.h"

int ioctl(int fd, unsigned long request, ...);

///////////////////////////////////////////////////////////////////////////////////////
//
// 
// 
int baudRates[13] = {110, 300, 600, 1200, 2400, 4800, 
                     9600, 14400, 19200, 31250, 38400, 57600, 
                     115200};

char * baudStr[4] = {" 0-110   1-300    2-600    3-1200",
                     " 4-2400  5-4800   6-9600   7-14400",  
                     " 8-19200 9-31250 10-38400 11-57600", 
                     "12-115200"};

///////////////////////////////////////////////////////////////////////////////////////
//
// int setbaud_set_baud(char * serialDevice, int fdSerial, int baud)
//
int setbaud_set_baud(char * serialDevice, int fdSerial, int baud)
{
    struct termios2 tio;
    misc_print(0, "Setting %s to %d baud.\n",serialDevice, baud);
    ioctl(fdSerial, TCGETS2, &tio);
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = baud;
    tio.c_ospeed = baud;
    ioctl(fdSerial, TCSETS2, &tio);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int setbaud_set_baud_31250(char * serialDevice)
// This changes the UART clock divisor to make 38400 BPS into 31250 BPS
// I'm not using this anymore but it does seem to work :)
int setbaud_set_baud_31250(char * serialDevice)
{
    char temp[100];
    sprintf(temp, "/bin/setserial -v %s spd_cust divisor 200", serialDevice);
    system(temp);
    return 0;
}
                         
///////////////////////////////////////////////////////////////////////////////////////
//
//  int setbaud_indexof(int baud)
// 

int setbaud_indexof(int baud)
{
    for(int index = 0; index < 13; index++)
        if (baudRates[index] == baud) return index;
    return -1;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int setbaud_baud_at_index(int index)
// 

int setbaud_baud_at_index(int index)
{
    if (index >= 0 && index <= 12)
        return baudRates[index];
    else
        return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int setbaud_is_valid_rate (int baud)
// 
int setbaud_is_valid_rate (int baud)
{
    /*
    if (setbaud_indexof(baud) != -1) 
        return TRUE;
    else
        return FALSE;
    */
    if (baud == 110   || baud == 300   || baud == 600   || baud == 1200  || baud == 2400  || baud == 4800  || 
        baud == 9600  || baud == 14400 || baud == 19200 || baud == 31250 || baud == 38400 || baud == 57600 || 
        baud == 115200)
        return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// setbaud_show_menu(int fdSerial)
// 
int setbaud_show_menu(int fdSerial)
{
    char line[] = "\r\n----------------------------------";
    write(fdSerial, line, sizeof(line));
    for (int index = 0; index < 4; index++)
    {
         write(fdSerial, "\r\n", 2);
         write(fdSerial, baudStr[index], strlen(baudStr[index]));
    }
    write(fdSerial, line, sizeof(line));
    char example1[] = "\r\nEXAMPLE --> ATBAUD9600";
    char example2[] = "\r\nEXAMPLE --> ATBAUD6";
    write(fdSerial, example1, strlen(example1));
    write(fdSerial, example2, strlen(example2));   
}


///////////////////////////////////////////////////////////////////////////////////////
//
// setbaud_show_menu(int fdSerial)
// 
int setbaud_getDSR(int fd)
{
    int s;
    /* Read terminal status line: Data Set Ready */
    ioctl(fd, TIOCMGET, &s);
    return (s & TIOCM_DSR) != 0;
}