#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include"misc.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial_set_interface_attribs(int fdSerial, int speed) to 38400 8N1 RTS/CTS flow control 
//
int serial_set_interface_attribs(int fdSerial)
{
    struct termios tty;

    if (tcgetattr(fdSerial, &tty) < 0)
    {
        misc_print(0, "ERROR: serial_set_interface_attribs() from tcgetattr --> %s\n", strerror(errno));
        return -1;
    }
    tty.c_cflag |= (CLOCAL | CREAD);               // ignore modem controls 
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;                            // 8-bit characters 
    tty.c_cflag &= ~PARENB;                        // no parity bit 
    tty.c_cflag &= ~CSTOPB;                        // only need 1 stop bit 
    tty.c_cflag &= ~(IXON | IXOFF );               // Disable XON/XOFF flowcontrol
    tty.c_cflag |= CRTSCTS;                        // CTS/RTS hardware flowcontrol 
    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    if (tcsetattr(fdSerial, TCSANOW, &tty) != 0)
    {
        misc_print(0, "ERROR: serial_set_interface_attribs() from tcsetattr --> %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int serial_set_flow_control(int fdSerial, int hayesMode) 
//
int serial_set_flow_control(int fdSerial, int hayesMode)
{
    char tmp [50] = "";
    struct termios tty;
    int VALID;    
    if (tcgetattr(fdSerial, &tty) < 0)
    {
        misc_print(0, "ERROR: serial_set_flow_control(%d) from tcgetattr --> %s\n", 
            hayesMode, strerror(errno));
        return FALSE;
    }
    switch (hayesMode)
    {  
        case 0:
            tty.c_cflag &= ~(CRTSCTS | IXON | IXOFF);
            VALID = TRUE;
            break;
        case 3:
            tty.c_cflag &= ~(IXON | IXOFF);
            tty.c_cflag |= CRTSCTS; 
            VALID = TRUE;
            break;
        case 4:   
            tty.c_cflag &= ~(CRTSCTS);
            tty.c_cflag |= IXON | IXOFF;
            VALID = TRUE;
            break;
        default:
            VALID = FALSE;
            if (hayesMode >= 0)
                misc_swrite(fdSerial, "\r\nUnsupported flow-Control --> '%d'", hayesMode);
            misc_swrite(fdSerial, "\r\n---------------------------"
                                  "\r\nSupported modes are:"
                                  "\r\n  0 - Disble flow-control"
                                  "\r\n  3 - RTS/CTS"
                                  "\r\n  4 - XON/XOFF");
    }
    if (VALID)
        if (tcsetattr(fdSerial, TCSANOW, &tty) != 0)
        {
            misc_print(0, "ERROR: serial_set_flow_control(%d) from tcsetattr -->  %s\n", 
                hayesMode, strerror(errno));
            return FALSE;
        }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//void serial_do_tcdrain(int fdSerial)
//
void serial_do_tcdrain(int fdSerial)
{
    tcdrain(fdSerial);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//void serial_set_timeout(int fdSerial, int timeout)
//
void serial_set_timeout(int fdSerial, int timeout)
{
    struct termios termios;
    //misc_print(0, "Serial set timeout --> %d\n",timeout);
    tcgetattr(fdSerial, &termios);
    termios.c_lflag &= ~ICANON;         // Set non-canonical mode 
    termios.c_cc[VTIME] = timeout * 10; // Set timeout seconds
    termios.c_cc[VMIN] = 0;
    tcsetattr(fdSerial, TCSANOW, &termios);
}

