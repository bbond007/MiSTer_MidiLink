#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ini.h"
#include "misc.h"

char                fsynthSoundFont[150];
extern char         UDPServer[50];
extern int          muntVolume;
extern int          fsynthVolume;
extern int          midilinkPriority;
extern unsigned int UDPServerPort;
extern unsigned int TCPServerPort;
extern int          UDPBaudRate;
extern int          TCPBaudRate;
extern unsigned int UDPServerFilterIP;
extern unsigned int DELAYSYSEX;

///////////////////////////////////////////////////////////////////////////////////////
//
// char ini_replace_char(char * str, int strLen, char old, char new)
//
char ini_replace_char(char * str, int strLen, char old, char new)
{
    for(int i = 0; i < strLen; i++)
        if(str[i] == old)
            str[i] = new;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char ini_process_key_value_pair(char * key, char * value)
//
char ini_process_key_value_pair(char * key, char * value)
{
    char * strPort, * endPtr;
    int iTmp;

    if(strcmp("MUNT_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            muntVolume = iTmp;
    }
    else if(strcmp("FSYNTH_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            fsynthVolume = iTmp;
    }
    else if(strcmp("UDP_SERVER_PORT", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            UDPServerPort = iTmp;
    }
    else if(strcmp("TCP_SERVER_PORT", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            TCPServerPort = iTmp;
    }
    else if (strcmp("UDP_SERVER", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(strlen(value) > 1)
            strcpy(UDPServer, value);
    }
    else if (strcmp("UDP_SERVER_FILTER", key) == 0)
    {
        if (strlen(value) > 0)
        {
            char c = toupper(value[0]);
            if (c == 'T' || c == 'Y')
                UDPServerFilterIP = TRUE;
            else
                UDPServerFilterIP = FALSE;
        }
        else
            UDPServerFilterIP = FALSE;
    }
    else if (strcmp("DELAYSYSEX", key) == 0)
    {
        if (strlen(value) > 0)
        {
            char c = toupper(value[0]);
            if (c == 'T' || c == 'Y')
                DELAYSYSEX = TRUE;
            else
                DELAYSYSEX = FALSE;
        }
        else
            DELAYSYSEX = FALSE;
    }
    else if (strcmp("FSYNTH_SOUNDFONT", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(strlen(value) > 1)
            strcpy(fsynthSoundFont, value);
    }
    else if (strcmp("MIDILINK_PRIORITY", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            midilinkPriority = iTmp;
    }
    else if (strcmp("UDP_BAUD", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            UDPBaudRate = iTmp;
    }
    else if (strcmp("TCP_BAUD", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            TCPBaudRate = iTmp;
    }
    else
        misc_print(0, "ERROR: ini_process_key_value() Unknown INI KEY --> '%s' = '%s'\n", key, value);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void ini_print_settings()
//
void ini_print_settings()
{
    misc_print(0, "Settings:\n");
    if(midilinkPriority != 0)
    misc_print(0, "  - MIDILINK_PRIORITY  --> %d\n",   midilinkPriority);
    else
    misc_print(0, "  - MIDILINK_PRIORITY  --> Default (don't change)\n");    
    if(muntVolume != -1)
    misc_print(0, "  - MUNT_VOLUME        --> %d%c\n", muntVolume, '%');
    else
    misc_print(0, "  - MUNT_VOLUME        --> Default (don't set)\n", muntVolume, '%');
    if(fsynthVolume != -1)
    misc_print(0, "  - FSYNTH_VOLUME      --> %d%c\n", fsynthVolume, '%');
    else
    misc_print(0, "  - FSYNTH_VOLUME      --> Default (don't set)\n", fsynthVolume, '%');
    misc_print(0, "  - FSYNTH_SOUNTFONT   --> '%s'\n", fsynthSoundFont);
    misc_print(0, "  - UDP_SERVER         --> '%s'%s\n", UDPServer,
        misc_ipaddr_is_multicast(UDPServer)?" MULTICAST":"");
    misc_print(0, "  - UDP_SERVER_PORT    --> %d\n",   UDPServerPort);
    if(UDPBaudRate > 0)
    misc_print(0, "  - UDP_BAUD           --> %d\n",   UDPBaudRate);
    else
    misc_print(0, "  - UDP_BAUD           --> Default (don't change)\n");
    misc_print(0, "  - UDP_SERVER_FILTER  --> %s\n",   UDPServerFilterIP?"TRUE":"FALSE");
    if(TCPBaudRate > 0)
    misc_print(0, "  - TCP_BAUD           --> %d\n",   TCPBaudRate);
    else
    misc_print(0, "  - TCP_BAUD           --> Default (don't change)\n");
    misc_print(0, "  - TCP_SERVER_PORT    --> %d\n",   TCPServerPort);
    misc_print(0, "  - DELAYSYSEX         --> %s\n",   DELAYSYSEX?"TRUE":"FALSE");
    misc_print(0, "\n");
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char ini_first_char(char * str, int len)
//
char ini_first_char(char * str, int len)
{
    for (int i = 0; i < len; i++)
        if (str[i] != ' ') return str[i];
    return 0x00;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int ini_parse_line(char * str, int len, char * key, int keyMax, char * value, int valMax)
//
int ini_parse_line(char * str, int len, char * key, int keyMax, char * value, int valMax)
{
    int iKey = 0;
    int iVal = 0;
    int eq =  FALSE;
    keyMax--;
    valMax--;

    for (int i = 0; i < len; i++)
        switch(str[i])
        {
        case '=':
            eq = TRUE;
            break;
        case ' ':
            if (eq && iVal)
                value[iVal++] = str[i];
            break;
        case 0x0a:
            break;
        case 0x0d:
            break;
        case 0x00:
            break;
        default :
            if(eq)
            {
                if(iVal < valMax)
                    value[iVal++] = str[i];
            }
            else
            {
                if(iKey < keyMax)
                    key[iKey++] = toupper(str[i]);
            }
        }
    key[iKey]   = 0x00;
    value[iVal] = 0x00;
    return eq;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int ini_read_loop (char * fileName, char * key, int keyMax, char * value, int valMax)
//
int ini_read_loop (char * fileName, char * key, int keyMax, char * value, int valMax)
{
    int count;
    char str[1024];
    FILE * file;
    file = fopen(fileName, "r");
    if (file)
    {
        while (fgets(str, sizeof(str), file)!= NULL)
        {
            if(ini_first_char(str, strlen(str)) != '#')
                if(ini_parse_line(str, strlen(str), key, keyMax, value, valMax))
                    ini_process_key_value_pair(key, value);
        }
        fclose(file);
        ini_print_settings();
        return TRUE;
    }
    else
    {
        misc_print(0, "ERROR: ini_read_loop() : Unable to open --> '%s'\n", fileName);
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int ini_read_ini(char * fileName)
//
int ini_read_ini(char * fileName)
{
    char key[30];
    char value[150];
    ini_read_loop(fileName, key, sizeof(key), value, sizeof(value));
}
