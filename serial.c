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
// int serial_set_interface_attribs(int fd, int speed) to 38400 8N1 RTS/CTS flow control 
//
int serial_set_interface_attribs(int fd)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0)
    {
        misc_print(0, "ERROR: serial_set_interface_attribs() from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

//    cfsetospeed(&tty, (speed_t)B38400);
//    cfsetispeed(&tty, (speed_t)B38400);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag |= CRTSCTS;     /* hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        misc_print(0, "ERROR: serial_set_interface_attribs() from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//void serial_do_tcdrain(int fd)
//
void serial_do_tcdrain(int fd)
{
    tcdrain(fd);
}
