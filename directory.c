#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <error.h>
#include <errno.h>
#include "ini.h"
#include "misc.h"
#include "serial.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL directory_read_loop (char * fileName, char * searchKey, char * key, int keyMax, char * value, int valMax, char * sec, int secMax)
//
int directory_read_loop (char * fileName, char * searchKey, char * key, int keyMax, char * value, int valMax, char * sec, int secMax)
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
                if(ini_parse_line(str, strlen(str), key, keyMax, value, valMax, sec, secMax))
                    if(strcmp(key, searchKey) == 0)
                    {
                        fclose(file);
                        return TRUE;
                    }
        }
        fclose(file);
        return FALSE;
    }
    else
    {
        misc_print(0, "ERROR: directory_read_loop() : Unable to open --> '%s'\n", fileName);
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL directory_search(char * fileName, char * searchKey, char * ipAddr)
//
int directory_search(char * fileName, char * searchKey, char * ipAddr)
{
    char key[30];
    char val[150];
    char sec[30];
    
    if (directory_read_loop(fileName, searchKey, key, sizeof(key), val, sizeof(val), sec, sizeof(sec)))
    {
        misc_print(1, "Directory search '%s' --> '%s'\n", searchKey, val);
        strcpy(ipAddr, val);
        return TRUE;
    }
    misc_print(1, "Directory search '%s' --> NOT FOUND\n", searchKey);
    return FALSE;
}
