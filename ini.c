#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ini.h"
#define TRUE  1
#define FALSE 0

char                fsynthSoundFont[150];
extern char         midiServer[50];
extern unsigned int muntVolume;
extern unsigned int fsynthVolume;
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
        if(iTmp > 0)
            muntVolume = iTmp;
    }
    else if(strcmp("FSYNTH_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp > 0)
            fsynthVolume = iTmp;
    }
    else if(strcmp("MIDI_SERVER_PORT", key) == 0)
    {
        iTmp = strtol(value, &endPtr, 10);
        if(iTmp > 0)
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
    printf("  - MUNT_VOLUME      --> %d%c\n", muntVolume, '%');
    printf("  - FSYNTH_VOLUME    --> %d%c\n", fsynthVolume, '%');
    printf("  - MIDI_SERVER      --> '%s'\n", midiServer);
    printf("  - MIDI_SERVER_PORT --> %d\n",   midiServerPort);
    printf("  - FSYNTH_SOUNTFONT --> '%s'\n", fsynthSoundFont);   
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
// int ini_split_line(char * str, int len, char * key, int keyLen, char * value, int valLen)
// 
int ini_split_line(char * str, int len, char * key, int keyLen, char * value, int valLen)
{
    int iKey = 0;
    int iVal = 0;
    int sep =  FALSE;
    keyLen--;
    valLen--;

    for (int i = 0; i < len; i++)
        switch(str[i])
        {
        case '=':
            sep = TRUE;
            break;

        case ' ':
            if (sep && iVal > 0)
                value[iVal++] = str[i];
            break;

        case 0x0a:
            break;
        case 0x0d:
            break;
        case 0x00:
            break;

        default :
            if(sep)
            {
                if(iVal < valLen)
                    value[iVal++] = str[i];
            }
            else
            {
                if(iKey < keyLen)
                    key[iKey++] = toupper(str[i]);
            }

        }
    key[iKey]   = 0x00;
    value[iVal] = 0x00;
    return sep;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int ini_read_loop (char * fileName, char * key, int keyLen, char * value, int valLen)
// 
int ini_read_loop (char * fileName, char * key, int keyLen, char * value, int valLen)
{
    int count;
    char str[999];
    FILE * file;
    file = fopen( fileName, "r");
    if (file)
    {
        while (fgets(str, sizeof(str), file)!= NULL)
        {
            if(ini_first_char(str, strlen(str)) != '#')
            {
                if(ini_split_line(str, strlen(str), key, keyLen, value, valLen))
                   ini_process_key_value(key, value);

            }
        }
        fclose(file);
        ini_print_settings();
    }

}

///////////////////////////////////////////////////////////////////////////////////////
//
// int ini_read_ini(char * fileName) 
// 
int ini_read_ini(char * fileName)
{
    char key[30];
    char value[50];
    ini_read_loop(fileName, key, sizeof(key), value, sizeof(value));
}
