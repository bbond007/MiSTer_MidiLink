#include <stdio.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include "openai.h"
#include "misc.h"
#include "jsmn.h"

//#define USE_JSMN //<-- TODO not working too well :(

char   OPENAI_KEY[150] = ""; //ini.c will populate this.
static CURL *curl = NULL;
static struct curl_slist *headers = NULL;
static size_t openai_callback(void *data, size_t size, size_t nmemb, void *clientp);
extern int  fdSerial;
extern enum ASCIITRANS TCPAsciiTrans;
struct openai_memory_chunk openai_chunk = {0};
static char STR_TEXT[]    = "text";
static char STR_MESSAGE[] = "message";

#ifdef USE_JSMN
static jsmn_parser jsmnParser;
static jsmntok_t   jsmnToken[1024]; /* We expect no more than 128 tokens */
#endif

///////////////////////////////////////////////////////////////////////////////////////
//
//  void openai_reset_chunk()
//
void openai_reset_chunk()
{
    free(openai_chunk.response);
    openai_chunk.response = malloc(0);
    openai_chunk.size = 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  CURL * openai_init()
//
CURL * openai_init()
{
    char auth_header[100];
    if(curl == NULL)
    {
        curl = curl_easy_init();
        if(curl)
        {
            snprintf(auth_header, sizeof(auth_header)-1, "Authorization: Bearer %s", OPENAI_KEY);
            misc_print(1, "DEBUG--> curl_slist_append()\n");
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header);
            misc_print(1, "DEBUG--> CURLOPT_URL\n");
            curl_easy_setopt(curl, CURLOPT_URL,"https://api.openai.com/v1/completions");
            misc_print(1, "DEBUG--> CURLOPT_POSTFIELDS\n");
            misc_print(1, "DEBUG--> CURLOPT_POSTFIELDS\n");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);
            misc_print(1, "DEBUG--> CURLOPT_HTTPHEADER\n");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            misc_print(1, "DEBUG--> CURLOPT_HTTPHEADER\n");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, openai_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&openai_chunk);
            misc_print(1, "DEBUG--> CURLOPT_WRITEFUNCTION\n");
        }
        return curl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  static void openai_handle_json_output(unsigned int iBracket, unsigned int iBrace, char * key, char * value)
//
static void openai_handle_json_output(unsigned int iBracket, unsigned int iBrace, char * key, char * value)
{
    misc_print(1, "DEBUG--> Entering openai_handle_json_output()\n");

    //misc_print(1, "DEBUG--> KEY = '%s' VALUE = '%s'\n", key, value);
    if (!strcasecmp(key, STR_TEXT) || !strcasecmp(key, STR_MESSAGE))
    {
        if (strcasecmp(key, "TEXT") == 0)
        {
            misc_swrite(fdSerial, "-->");
            //this segfaults for long value? :(
            // WTF? WHY? --> TODO <--
            //misc_swrite(fdSerial, value);
            switch (TCPAsciiTrans)
            {
            case AsciiToPetskii:
                misc_ascii_to_petskii_null(value);
                break;
            }
            write(fdSerial, value, strlen(value));
        }
        else
        {
            misc_swrite(fdSerial, "ERROR--> %s\r\n", value);
            misc_swrite(fdSerial, "Don't forget to set OPENAI_KEY = ##### in MidiLink.INI.\r\n");
        }
    }
    misc_print(0, "DEBUG--> Exiting openai_handle_json_output()\n");
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  size_t openai_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
//
static size_t openai_callback(void *data, size_t size, size_t nmemb, void *clientp)
{
    size_t realsize = size * nmemb;
    struct openai_memory_chunk *mem = (struct openai_memory_chunk *)clientp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;  /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    misc_print(1, "DEBUG--> Getting openai response.\n");
    return realsize;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  CURLcode openai_say(char * msg)
//

#ifdef USE_JSMN
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0)
    {
        return 0;
    }
    return -1;
}
#endif

CURLcode openai_say(char * msg)
{
    if(strlen(msg) < 3)
        return -1;

    char   formatStr[] = "{\"model\":\"text-davinci-003\",\"prompt\":\"%s\",\"max_tokens\":1024,\"temperature\":0.5}";
    size_t bufOutSize = strlen(msg) + sizeof(formatStr);
    char * bufOut = malloc(bufOutSize);
    if (bufOut)
    {
        if (!curl)
            openai_init();
        if (curl)
        {
            CURLcode cRes;
            snprintf(bufOut, bufOutSize - 1, formatStr, msg);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bufOut);
            cRes = curl_easy_perform(curl);
            if(cRes != CURLE_OK)
                misc_print(0, "curl_easy_perform() failed: %s\n",
                           curl_easy_strerror(cRes));
#ifdef USE_JSMN
d
DEBUG--> curl_slist_append()
DEBUG--> CURLOPT_URL
            int iRes = jsmn_parse(&jsmnParser, openai_chunk.response, openai_chunk.size, jsmnToken,
                               sizeof(jsmnToken) / sizeof(jsmnToken[0]));;
            if (iRes > -1)
            {
                char * key;
                for (int i = 1; i < iRes; i++)
                {
                    if (jsoneq(openai_chunk.response, &jsmnToken[i], STR_TEXT) == 0)
                        key = STR_TEXT;
                    else if (jsoneq(openai_chunk.response, &jsmnToken[i], STR_MESSAGE) == 0)
                        key = STR_MESSAGE;
                    else
                        key = "";

                    if (key[0] != 0x00)
                    {
                        size_t valSize = jsmnToken[i + 1].end - jsmnToken[i + 1].start;
                        char * value = malloc(valSize + 1);
                        if (value)
                        {
                            memcpy(value, openai_chunk.response + jsmnToken[i + 1].start, valSize);
                            value[valSize] = 0x00;
                            openai_handle_json_output(0,0, key, value);
                            free(value);
                        }
                        else
                            misc_print(0, "ERROR--> value = malloc(%d)!\n", valSize + 1);
                    }
                }
            }
            else
                misc_print(0, "ERROR--> jsmn_parse() fail (iRres=%d)!\n", iRes);
#else
            openai_parse_json_content(openai_chunk.response, openai_chunk.size, TRUE);
#endif
            openai_reset_chunk();
            return cRes;
        }
        else
        {
            misc_print(0, "ERROR--> curl == NULL!!!\n");
            return -2;
        }
        free(bufOut);
    }
    else
    {
        misc_print(0, "ERROR--> bufOut = malloc(%d)!\n", bufOutSize);
        return -3;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
//  void openai_done()
//
void openai_done()
{
    if (curl)
    {
        misc_print(1, "curl_slist_free_all()\n");
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl = NULL;
        headers = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
//  void openai_parse_json_content(char * jsonContent, bool reset)
//  void openai_parse_json_content(char * jsonContent, size_t jsonContentSize, bool reset)
//
#ifdef USE_JSMN
#else
void openai_parse_json_content(char * jsonContent, size_t jsonContentSize, bool reset)
{
    char * c = jsonContent;
    static unsigned int iBracket    = 0;
    static unsigned int iBrace      = 0;
    static unsigned int iKey        = 0;
    static unsigned int iValue      = 0;
    static bool quote               = false;
    static bool findkey             = true;
    static char key[128]            = "";
    static char value[1024 * 1024]  = "";
    static char lastc               = 0x00;
    if(reset)
    {
        quote    = false;
        key[0]   = 0x00;
        value[0] = 0x00;
        iKey     = 0;
        iValue   = 0;
        findkey  = false;
        iBracket = 0;
        iBrace   = 0;
    }

    misc_print(0, "DEBUG--> Entering openai_parse_json_content()\n");
    //while(*c != 0x00)
    for(int index = 0; index < jsonContentSize; index++)
    {
        bool bSkip = false;
        if (*c == '"' && lastc != BSL_KEY )
        {
            quote = !quote;
            bSkip = true;
        }

        if (*c == BSL_KEY && lastc != BSL_KEY)
            bSkip = true;

        if (!quote)
        {
            switch (*c)
            {

            case '{' :
                iBracket++;
                findkey   = true;
                iKey      = 0;
                key[0]    = 0x00;
                bSkip     = true;
                break;

            case '[' :
                findkey   = true;
                iKey      = 0;
                key[0]    = 0x00;
                iBrace++;
                bSkip     = true;
                break;

            case ']' :
                iBrace--;
                bSkip     = true;
                break;

            case '}' :
                openai_handle_json_output(iBracket, iBrace, key, value);
                findkey  = true;
                iKey     = 0;
                key[0]   = 0x00;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                iBracket--;
                break;

            case ',' :

                openai_handle_json_output(iBracket, iBrace, key, value);
                findkey  = true;
                iKey     = 0;
                key[0]   = 0x00;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                break;

            case '\n' :
                bSkip    = true;
                break;

            case '\r' :
                bSkip    = true;
                break;

            case '\t':
                bSkip    = true;
                break;

            case ':' :
                findkey  = false;
                iValue   = 0;
                value[0] = 0x00;
                bSkip    = true;
                break;
            }
        }

        if(!bSkip)
        {
            if(findkey)
            {
                if (quote && iKey < sizeof(key)-1)
                {
                    key[iKey++] = *c;
                    key[iKey] = 0x00;
                }
            }
            else
            {
                if(iValue < sizeof(value)-2)
                {
                    if (*c == 'n' && lastc == BSL_KEY)
                    {
                        value[iValue++] = 0x0a;
                        value[iValue++] = 0x0d;
                        value[iValue] = 0x00;
                    }
                    else if(*c != ' ' || quote)
                    {
                        value[iValue++] = *c;
                        value[iValue] = 0x00;
                    }
                }
            }
        }
        lastc = *c;
        c++;
    }
    misc_print(0, "DEBUG--> Exiting openai_parse_json_content()");
}
#endif

