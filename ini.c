#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ini.h"
#define TRUE  1
#define FALSE 0

char                fsynthSoundFont[150];
extern char         midiServer[50];
extern int          muntVolume;
extern int          fsynthVolume;
extern int          midilinkPriority;  
extern unsigned int midiServerPort;

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
// char ini_process_key_value(char * key, char * value)
// 
char ini_process_key_value(char * key, char * value)
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
    else if(strcmp("MIDI_SERVER_PORT", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp != 0)
            midiServerPort = iTmp;
    }
    else if (strcmp("MIDI_SERVER", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(strlen(value) > 1)
            strcpy(midiServer, value);
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
    else
        printf("ERROR Unknown INI KEY --> '%s' = '%s'\n", key, value);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void ini_print_settings()
// 
void ini_print_settings()
{
    printf("Settings:\n");
    if(muntVolume != -1)
    printf("  - MUNT_VOLUME       --> %d%c\n", muntVolume, '%');
    else
    printf("  - MUNT_VOLUME       --> Default (don't set)\n", muntVolume, '%');
    if(fsynthVolume != -1)
    printf("  - FSYNTH_VOLUME     --> %d%c\n", fsynthVolume, '%');
    else
    printf("  - FSYNTH_VOLUME     --> Default (don't set)\n", fsynthVolume, '%');
    printf("  - MIDI_SERVER       --> '%s'\n", midiServer);
    printf("  - MIDI_SERVER_PORT  --> %d\n",   midiServerPort);
    printf("  - FSYNTH_SOUNTFONT  --> '%s'\n", fsynthSoundFont);   
    if(midilinkPriority != 0)
    printf("  - MIDILINK_PRIORITY --> %d\n",   midilinkPriority);
    else
    printf("  - MIDILINK_PRIORITY --> Default (don't change)\n", midilinkPriority);   
    printf("\n");
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
    char str[999];
    FILE * file;
    file = fopen(fileName, "r");
    if (file)
    {
        while (fgets(str, sizeof(str), file)!= NULL)
        {
            if(ini_first_char(str, strlen(str)) != '#')
            {
                if(ini_parse_line(str, strlen(str), key, keyMax, value, valMax))
                   ini_process_key_value(key, value);
            }
        }
        fclose(file);
        ini_print_settings();
        return TRUE;
    }
    else
    {
        printf("ERROR ini_read_loop() : Unable to open --> '%s'\n", fileName);
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
