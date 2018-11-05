#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define TRUE  1
#define FALSE 0


///////////////////////////////////////////////////////////////////////////////////////
//
// args_str_to_upper(char *str)
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
// args_check_option (int argc, char *argv[], char * option)
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
