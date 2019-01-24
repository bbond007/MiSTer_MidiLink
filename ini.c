#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <error.h>
#include <errno.h>
#include "ini.h"
#include "misc.h"
#include "serial.h"

extern char         fsynthSoundFont[150];
extern char         UDPServer[100];
extern char         mixerControl[20]; 
extern int          MP3Volume;
extern int          muntVolume;
extern int          fsynthVolume;
extern int          midilinkPriority;
extern unsigned int UDPServerPort;
extern unsigned int TCPServerPort;
extern unsigned int TCPTermRows;
extern int          UDPBaudRate;
extern int          TCPBaudRate;
extern int          TCPSoftSynth;
extern int          TCPFlow;
extern int          UDPFlow;
extern unsigned int UDPServerFilterIP;
extern unsigned int DELAYSYSEX;
extern char         MP3Path[500];
extern char 	    MIDIPath[500];
extern char         downloadPath[500];
extern char         uploadPath[100];
extern char         MUNTOptions[30];

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
// void ini_bool(char * value, int * dest)
//
void ini_bool(char * value, int * dest)
{
    if (strlen(value) > 0)
    {
        char c = toupper(value[0]);
        if (c == 'T' || c == 'Y')
            *dest = TRUE;
        else
            *dest = FALSE;
    }
    else
        *dest = FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void int_str(char * dest)
//
void ini_str(char * value, char * dest)
{
   if(strlen(value) > 1)     
       strcpy(dest, value);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void ini_int(char * value, int * dest)
//
void ini_int(char * value, int * dest)
{
  char * endPtr;
  int iTmp = strtol(value, &endPtr, 10);
  if(*endPtr == (char) 0x00)
      *dest = iTmp;
}
 
///////////////////////////////////////////////////////////////////////////////////////
//
// void ini_uint(char * value, unsigned int * iValue )
//
void ini_uint(char * value, unsigned int * dest )
{
  char * endPtr;
  unsigned int iTmp = strtol(value, &endPtr, 10);
  if(*endPtr == (char) 0x00)
       *dest = iTmp;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// char ini_process_key_value_pair(char * key, char * value)
//
char ini_process_key_value_pair(char * key, char * value)
{
    if(strcmp("MP3_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        ini_int(value, &MP3Volume);
    }
    if(strcmp("MUNT_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        ini_int(value, &muntVolume);
    }
    else if(strcmp("FSYNTH_VOLUME", key) == 0)
    {
        ini_replace_char(value, strlen(value), '%', 0x00);
        ini_int(value, &fsynthVolume);
    }
    else if (strcmp("MIXER_CONTROL", key) == 0)
    {
        ini_str(value, mixerControl);
    }
    else if(strcmp("UDP_SERVER_PORT", key) == 0)
    {
        ini_uint(value, &UDPServerPort);
    }
    else if(strcmp("TCP_SERVER_PORT", key) == 0)
    {
        ini_uint(value, &TCPServerPort);
    }
    else if (strcmp("UDP_SERVER", key) == 0)
    {
        ini_str(value, UDPServer);
    }
    else if (strcmp("UDP_SERVER_FILTER", key) == 0)
    {
        ini_bool(value, &UDPServerFilterIP);
    }
    else if (strcmp("DELAYSYSEX", key) == 0)
    {
        ini_bool(value, &DELAYSYSEX);
    }
    else if (strcmp("FSYNTH_SOUNDFONT", key) == 0)
    {
        ini_str(value, fsynthSoundFont);
    }
    else if (strcmp("MIDILINK_PRIORITY", key) == 0)
    {
        ini_int(value, &midilinkPriority);
    }
    else if (strcmp("UDP_BAUD", key) == 0)
    {
        ini_int(value, &UDPBaudRate);
    }
    else if (strcmp("TCP_BAUD", key) == 0)
    {
        ini_int(value, &TCPBaudRate);
    }
    else if (strcmp("TCP_TERM_ROWS", key) == 0)
    {
        ini_uint(value, &TCPTermRows);
    }
    else if (strcmp("TCP_TERM_UPLOAD", key) == 0)
    {
        ini_str(value, uploadPath);
    }
    else if (strcmp("TCP_TERM_DOWNLOAD", key) == 0)
    {
        ini_str(value, downloadPath);
    }
    else if (strcmp("TCP_TERM_MP3", key) == 0)
    {
        ini_str(value, MP3Path);
    }
    else if (strcmp("TCP_TERM_MIDI", key) == 0)
    {
        ini_str(value, MIDIPath);
    }
    else if (strcmp("TCP_FLOW", key) == 0)
    {
        ini_int(value, &TCPFlow);   
    }
    else if (strcmp("UDP_FLOW", key) == 0)
    {
        ini_int(value, &UDPFlow);
    }
    else if (strcmp("MUNT_OPTIONS", key) == 0)
    {
        ini_str(value, MUNTOptions);
    }
    else if (strcmp("TCP_TERM_SYNTH", key) == 0)
    {
        if(strlen(key) > 0)
            switch(toupper(value[0]))
            {	
                case 'M': TCPSoftSynth = MUNT;
                    break;
                case 'F': TCPSoftSynth = FluidSynth;
                    break;
                default:  TCPSoftSynth = FluidSynth;
                    break;
            }
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
    misc_print(0, "  - MUNT_OPTIONS       --> '%s'\n", MUNTOptions);    
    if(MP3Volume != -1)
    misc_print(0, "  - MP3_VOLUME         --> %d%c\n", MP3Volume, '%');
    else
    misc_print(0, "  - MP3_VOLUME         --> Default (don't set)\n");
    if(muntVolume != -1)
    misc_print(0, "  - MUNT_VOLUME        --> %d%c\n", muntVolume, '%');
    else
    misc_print(0, "  - MUNT_VOLUME        --> Default (don't set)\n");
    if(fsynthVolume != -1)
    misc_print(0, "  - FSYNTH_VOLUME      --> %d%c\n", fsynthVolume, '%');
    else
    misc_print(0, "  - FSYNTH_VOLUME      --> Default (don't set)\n");
    misc_print(0, "  - MIXER_CONTROL      --> %s\n",   mixerControl);
    misc_print(0, "  - FSYNTH_SOUNTFONT   --> '%s'\n", fsynthSoundFont);
    misc_print(0, "  - UDP_SERVER         --> '%s'%s\n", UDPServer,
        misc_ipaddr_is_multicast(UDPServer)?" MULTICAST":"");
    misc_print(0, "  - UDP_SERVER_PORT    --> %d\n",   UDPServerPort);
    if(UDPBaudRate > 0)
    misc_print(0, "  - UDP_BAUD           --> %d\n",   UDPBaudRate);
    else
    misc_print(0, "  - UDP_BAUD           --> Default (don't change)\n");
    misc_print(0, "  - UDP_SERVER_FILTER  --> %s\n",   UDPServerFilterIP?"TRUE":"FALSE");
    if(UDPFlow != -1)
    misc_print(0, "  - UDP_FLOW           --> (%d) %s\n", UDPFlow, serial_hayes_flow_to_str(UDPFlow));
    else
    misc_print(0, "  - UDP_FLOW           --> Default (don't change)\n");
    if(TCPBaudRate > 0)
    misc_print(0, "  - TCP_BAUD           --> %d\n",   TCPBaudRate);
    else
    misc_print(0, "  - TCP_BAUD           --> Default (don't change)\n");
    if(TCPFlow != -1)
    misc_print(0, "  - TCP_FLOW           --> (%d) %s\n", TCPFlow, serial_hayes_flow_to_str(TCPFlow));
    else
    misc_print(0, "  - TCP_FLOW           --> Default (don't change)\n");
    misc_print(0, "  - TCP_SERVER_PORT    --> %d\n",   TCPServerPort);
    misc_print(0, "  - TCP_TERM_ROWS      --> %d\n",   TCPTermRows);
    misc_print(0, "  - TCP_TERM_UPLOAD    --> %s\n",   uploadPath);
    misc_print(0, "  - TCP_TERM_DOWNLOAD  --> %s\n",   downloadPath);
    misc_print(0, "  - TCP_TERM_MP3       --> %s\n",   MP3Path);
    misc_print(0, "  - TCP_TERM_MIDI      --> %s\n",   MIDIPath);
    misc_print(0, "  - TCP_TERM_SYNTH     --> %s\n",  (TCPSoftSynth==MUNT)?"MUNT":"FluidSynth");
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
        case 0x0a: // [NEW LINE]
        case 0x0d: // [RETURN]
        case 0x00: // [NULL]
        case '\t': // [TAB]
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
