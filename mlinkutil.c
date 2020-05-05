#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "serial.h"
#include "serial2.h"
#include "misc.h"
#include "config.h"


#define DEFAULT_TCPAsciiTrans  AsciiNoTrans

int                     MIDI_DEBUG             = TRUE;
enum ASCIITRANS         TCPAsciiTrans          = DEFAULT_TCPAsciiTrans;
static int              fdSerial               = -1;


///////////////////////////////////////////////////////////////////////////////////////
//
// int main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{
    int result = misc_check_args_option(argc, argv, "BAUD");
    
    if(result && (result + 1 < argc))
    {
         char * ptr;
         int baud = strtol(argv[result + 1], &ptr, 10);   
         if (!serial2_is_valid_rate (baud))
         {       
             printf("ERROR : BAUD not valid --> %s\n", argv[result + 1]);
             return -1; 
         }
         //printf("SET BAUD --> %d\n", baud);
         fdSerial = open(serialDevice, O_RDWR | O_NOCTTY | O_SYNC);
         if (fdSerial < 0)
         {
              misc_print(0, "ERROR: opening %s: %s\n", serialDevice, strerror(errno));
              return -2;    
         }
         serial2_set_baud(serialDevice, fdSerial, baud);
         close(fdSerial);
    }
}
