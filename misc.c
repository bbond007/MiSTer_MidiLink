#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h> // close function




#define TRUE  1
#define FALSE 0

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
    printf("Checking for --> %s : ", deviceName);
    if (stat(deviceName, &filestat) != 0)
    {
        printf("FALSE\n");
        return FALSE;
    }
    printf("TRUE\n"); 
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
// bool get_server_and_port(char * server)
//

int misc_get_server_and_port(char * server, int * pport)
{
    char buffer[30];
    int port = -1;
    char * strPort, * endPtr;
    char MIDISERVER[] = "/media/fat/MIDISERVER";
    int fdMidiServer;  
    fdMidiServer = open (MIDISERVER, O_RDONLY);
    if (fdMidiServer == -1) 
    {
        printf("ERROR unable to open %s\n", MIDISERVER);
        return -1;
    }
    int rdLen = read(fdMidiServer, buffer, sizeof(buffer));
    if (rdLen < 7)
    {

        printf("%s read length error\n",MIDISERVER);
        close(fdMidiServer);
        return -1; 
    }

    char * c = strchr(buffer, ':');
    if (c != NULL) 
    {
        *c = (char) 0x00;
        c++;
        strPort = c;
        port = strtol(strPort, &endPtr, 10);
    }
    close(fdMidiServer);
    //printf("MIDI Server Address --> %s\n", buffer);
    //printf("MIDI Server Port --> %d\n", port);
    strcpy(server, buffer);
    *pport = port;
    return TRUE;
}
