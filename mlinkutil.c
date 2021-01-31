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
#include "tcpsock.h"


#define DEFAULT_TCPAsciiTrans  AsciiNoTrans

int                     MIDI_DEBUG             = TRUE;
enum ASCIITRANS         TCPAsciiTrans          = DEFAULT_TCPAsciiTrans;
int                     fdSerial               = -1;
int                     socket_out             = -1;


///////////////////////////////////////////////////////////////////////////////////////
//
// int main(int argc, char *argv[])
//
int main(int argc, char *argv[])
{
    int bShowInfo = TRUE;
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
        char strBaud[10];
        sprintf(strBaud,"%d", baud);
        misc_make_file(tmpBAUD, strBaud);
        serial2_set_baud(serialDevice, fdSerial, baud);
        close(fdSerial);
        
        if(misc_check_device(serialDeviceUSB) && misc_check_file("/tmp/ML_USBSER"))
        {
            fdSerial = open(serialDeviceUSB, O_RDWR | O_NOCTTY | O_SYNC);
            if (fdSerial < 0)
            {
                misc_print(0, "ERROR: opening %s: %s\n", serialDeviceUSB, strerror(errno));
                return -3;
            }
            serial2_set_baud(serialDeviceUSB, fdSerial, baud);
            close(fdSerial);
        }
        bShowInfo = FALSE;
    }

    result = misc_check_args_option(argc, argv, "FSSFONT");
    if(result && (result + 1 < argc))
    {
        if(misc_check_file(argv[result + 1]))
        {
            socket_out = tcpsock_client_connect("127.0.0.1", 9800, fdSerial);
            if (socket_out > 0)
            {
                char sListSF[] = "fonts\n";
                tcpsock_write(socket_out, sListSF, strlen(sListSF));
                sleep(1);

                char buf[1024];
                int TCPresult = tcpsock_read(socket_out,  buf,  sizeof(buf));
                if(TCPresult > 15)
                {
                    buf[TCPresult] = 0x00;
                    if (strncmp("ID Name", buf, 7))
                    {
                        char * sfNo = &buf[9];
                        if (sfNo[0] == ' ') sfNo++;
                        char * tm = strchr(sfNo, ' ');
                        if (tm) *tm = 0x00;
                        if (strchr("1234567890", sfNo[0]))
                        {
                            printf("Unload Sounfont #%s --> '%s'\n", sfNo, tm + 1);
                            char sUnloadSF[30];
                            sprintf(sUnloadSF, "unload %s\n", sfNo);
                            tcpsock_write(socket_out, sUnloadSF, strlen(sUnloadSF));
                            sleep(1);
                        }
                    } 
                }
                printf("Sending --> RESET\n");
                tcpsock_write(socket_out, "reset\n", 6);
                char sLoadSF[250];
                sprintf(sLoadSF, "load \"%s\"\n", argv[result + 1]);
                printf("Loading SoundFont --> '%s'\n", argv[result + 1]);
                tcpsock_write(socket_out, sLoadSF, strlen(sLoadSF));
                //sleep(1);
                close(socket_out);
            }
            else
            {	
                printf("ERROR --> unable to connect to FluidSynth:9800\n");
                return -11;
            }
            misc_make_file(tmpSoundfont, argv[result + 1]); 
            bShowInfo = FALSE;
        }
        else
        {
            printf("ERROR --> unable to open file '%s'\n", argv[result+1]);
            return -10;
        }
    }
    if (bShowInfo)
        printf("%s\nUsage:\n - #mlinkutil BAUD [rate]\n - #mlinkutil FSSFONT [fileaname]\n",helloStr);
    return 0;
}
