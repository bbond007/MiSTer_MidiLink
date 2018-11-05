#include <stdlib.h>
#include <string.h>
#include <asm/termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int ioctl(int fd, unsigned long request, ...);


// I put this in its own file because /asm/termios.h conflicts with termios.h


///////////////////////////////////////////////////////////////////////////////////////
//
// int setbaud_set_baud(char * serialDevice, int fdSerial, int baud)
//

int setbaud_set_baud(char * serialDevice, int fdSerial, int baud)
{
    struct termios2 tio;
    printf("Setting %s to %d baud.\n",serialDevice, baud);
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