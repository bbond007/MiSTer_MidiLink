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
// int serial2_set_baud(char * serialDevice, int fdSerial, int baud)
//
int serial2_set_baud(char * serialDevice, int fdSerial, int baud)
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
// int serial2_set_baud_31250(char * serialDevice)
// This changes the UART clock divisor to make 38400 BPS into 31250 BPS
// I'm not using this anymore but it does seem to work :)
int serial2_set_baud_31250(char * serialDevice)
{
    char temp[100];
    sprintf(temp, "/bin/setserial -v %s spd_cust divisor 200", serialDevice);
    system(temp);
    return 0;
}
                         
///////////////////////////////////////////////////////////////////////////////////////
//
//  int serial2_indexof(int baud)
// 

int serial2_indexof(int baud)
{
    for(int index = 0; index < 13; index++)
        if (baudRates[index] == baud) return index;
    return -1;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial2_baud_at_index(int index)
// 

int serial2_baud_at_index(int index)
{
    if (index >= 0 && index <= 12)
        return baudRates[index];
    else
        return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial2_is_valid_rate (int baud)
// 
int serial2_is_valid_rate (int baud)
{
    /*
    if (serial2_indexof(baud) != -1) 
        return TRUE;
    else
        return FALSE;
    */
    if (baud == 110   || baud == 300   || baud == 600   || baud == 1200  || baud == 2400  || baud == 4800  || 
        baud == 9600  || baud == 14400 || baud == 19200 || baud == 31250 || baud == 38400 || baud == 57600 || 
        baud == 115200 ||
        baud == 93750)
        return TRUE;
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// serial2_show_menu(int fdSerial)
// 
int serial2_show_menu(int fdSerial)
{
    char line[] = "\r\n----------------------------------";
    misc_swrite(fdSerial, line);
    for (int index = 0; index < 4; index++)
    {
         misc_swrite(fdSerial,"\r\n");
         misc_swrite(fdSerial, baudStr[index]);
    }
    misc_swrite(fdSerial, line);
    misc_swrite(fdSerial, "\r\nEXAMPLE --> ATBAUD9600");
    misc_swrite(fdSerial, "\r\nEXAMPLE --> ATBAUD6");
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial2_get_DSR(int fdSerial)
// 
int serial2_get_DSR(int fdSerial)
{
    int s;
    /* Read terminal status line: Data Set Ready */
    ioctl(fdSerial, TIOCMGET, &s);
    return (s & TIOCM_DSR) != 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial2_set_DTR(int fdSerial, int on)
// 
int serial2_set_DTR(int fdSerial, int on)
{
    int controlbits = TIOCM_DTR;
    /* Set terminal status line: Data Set Ready */
    return ioctl(fdSerial, on?TIOCMBIS:TIOCMBIC, &controlbits);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial2_set_DCD(char * serialDevice, int fdSerial, int on)
// 
int serial2_set_DCD(char * serialDevice, int fdSerial, int on)
{
    int result;
    result = serial2_set_DTR(fdSerial, on);
    misc_print(1, "Setting DCD (%s) --> %s\n", serialDevice, on?"TRUE":"FALSE");
    return result; 
}
