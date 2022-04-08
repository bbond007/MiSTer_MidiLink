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
// int compare(char * searchKey, char * key, char * vlaue, char * sec)
//

int compare(char * searchKey, char * key, char * vlaue)
{
    if (strcmp(key, searchKey) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int output(char* searchKey, char* key, char* value)
//
static int _rows;
static int _fdSerial;
static int _rowCount = 0;
int output(char* searchKey, char* key, char* value)
{
    char c = (char)0x00;
    misc_swrite(_fdSerial, "%s --> %s\r\n", key, value);
    misc_do_rowcheck(_fdSerial, _rows, &_rowCount, &c);
    if (c == 'Q' || c == 'q')
        return TRUE;
    else
        return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
// BOOLint directory_read_loop (int (*fun_ptr)(char * searchKey, char * key, char * value),
//                              char* fileName,
//                              char* section,
//                              char* searchKey,
//                              char* key, int keyMax,
//                              char* value, int valMax,
//                              char* sec, int secMax)

int directory_read_loop (int (*fun_ptr)(char * searchKey, char * key, char * value),
                         char * fileName, 
                         char * section,  
                         char * searchKey,
                         char * key,   int keyMax, 
                         char * value, int valMax, 
                         char * sec,   int secMax)
{
    int count;
    char str[1024];
    FILE * file;
	int result = FALSE;
	sec[0] = 0x00;
    file = fopen(fileName, "r");
    if (file)
    {
        while (fgets(str, sizeof(str), file) != NULL)
        {
            if (ini_first_char(str, strlen(str)) != '#')
                if (ini_parse_line(str, strlen(str), key, keyMax, value, valMax, sec, secMax))
                    if (sec[0] == 0x00 || strcmp(section, sec) == 0)
                        if (result = fun_ptr(searchKey, key, value))
                            break;
        }
        fclose(file);
        return result;
    }
    else
    {
        misc_print(0, "ERROR: directory_read_loop() : Unable to open --> '%s'\n", fileName);
        return FALSE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// BOOL directory_search(char * fileName, char * coreName, char * searchKey, char * ipAddr)
//
int directory_search(char * fileName, char * coreName, char * searchKey, char * ipAddr)
{
    char key[30];
    char val[150];
    char sec[30];
    
    if (directory_read_loop(compare, 
                            fileName, 
                            coreName, 
                            searchKey, 
                            key, sizeof(key), 
                            val, sizeof(val), 
                            sec, sizeof(sec)))
    {
        misc_print(1, "Directory search '%s' --> '%s'\n", searchKey, val);
        strcpy(ipAddr, val);
        return TRUE;
    }
    misc_print(1, "Directory search '%s' --> NOT FOUND\n", searchKey);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// VOID directory_list(int fdSerial, int rows, char* fileName, char* coreName);
//
void directory_list(int fdSerial, int rows, char* fileName, char* coreName)
{
    char key[30];
    char val[150];
    char sec[30];
    _rows     = rows;
    _fdSerial = fdSerial;
    _rowCount = 0;
    misc_swrite(_fdSerial, "\r\n");
    directory_read_loop(output,
                        fileName,
                        coreName,
                        "",
                        key, sizeof(key),
                        val, sizeof(val),
                        sec, sizeof(sec));
}

