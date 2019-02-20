#include <alsa/asoundlib.h>
#include <math.h>
#include <ctype.h>

#include "misc.h"

#define RATE 8000
#define DIAL_VOLUME 70
#define CONNECT_VOLUME 100

///////////////////////////////////////////////////////////////////////////////////////
//int modem_snd_play_digit(snd_pcm_t *handle,
//                         char * buf,
//                         int bufLen,
//                         int tone1,
//                         int tone2)

int modem_snd_play_digit(snd_pcm_t *handle,
                         char * buf,
                         int bufLen,
                         int tone1,
                         int tone2)

{
    int err;
    snd_pcm_sframes_t frames;
    double cost1 = 2.0 * M_PI * tone1 / (double) RATE;
    double cost2 = 2.0 * M_PI * tone2 / (double) RATE;
    for (int i = 0; i < bufLen ; i++)
    {
        double p1 = sin (i * cost1);
        double p2 = sin (i * cost2);
        unsigned char t1 = (p1 * DIAL_VOLUME);
        unsigned char t2 = (p2 * DIAL_VOLUME);
        buf[i] = t1 & t2;
    }
//  snd_pcm_drain(handle);
    frames = snd_pcm_writei(handle, buf, bufLen);
    if (frames < 0)
        frames = snd_pcm_recover(handle, frames, 0);
    if (frames < 0)
    {
        misc_print(0, "ERROR: modem_snd_play_digit() --> snd_pcm_writei failed: %s\n", snd_strerror(frames));
        return -1;
    }
    if (frames > 0 && frames < (long) bufLen)
        misc_print(0, "ERROR: modem_snd_play_digit() --> Short write (expected %li, wrote %li)\n", (long) bufLen, frames);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//int modem_snd_play_random(snd_pcm_t *handle,
//                          char * buf,
//                          int bufLen)
int modem_snd_play_random(snd_pcm_t *handle,
                          char * buf,
                          int bufLen,
                          int max)
{
    int err;
    snd_pcm_sframes_t frames;
    for (int i = 0; i < bufLen; i++)
    {
        buf[i] = rand() % max;
    }
    frames = snd_pcm_writei(handle, buf, bufLen);
    if (frames < 0)
        frames = snd_pcm_recover(handle, frames, 0);
    if (frames < 0)
    {
        misc_print(0, "ERROR: modem_snd_play_random() --> snd_pcm_writei failed: %s\n", snd_strerror(frames));

        return -1;
    }
    if (frames > 0 && frames < (long) bufLen)
        misc_print(0,"ERROR: modem_snd_play_random() --> Short write (expected %li, wrote %li)\n", (long) bufLen, frames);
}
                           
///////////////////////////////////////////////////////////////////////////////////////
//int modem_snd_play_number(snd_pcm_t *handle,
//                          char * buf,
//                          int bufLen,
//                          char * number)

int modem_snd_play_number(snd_pcm_t *handle,
                          char * buf,
                          int bufLen,
                          char * number)
{
    int tone1;
    int tone2;
    while(*number != (char) 0x00)
    {
        tone1 = 0;
        switch(toupper(*number))
        {
        case '0':
            tone2 = 941;
            tone1 = 1366;
            break;
        case '1':
            tone2 = 697;
            tone1 = 1209;
            break;
        case '2':
            tone2 = 697;
            tone1 = 1366;
            break;
        case '3':
            tone2 = 697;
            tone1 = 1477;
            break;
        case '4':
            tone2 = 770;
            tone1 = 1209;
            break;
        case '5':
            tone2 = 770;
            tone1 = 1366;
            break;
        case '6':
            tone2 = 770;
            tone1 = 1477;
            break;
        case '7':
            tone2 = 852;
            tone1 = 1209;
            break;
        case '8':
            tone2 = 852;
            tone1 = 1366;
            break;
        case '9':
            tone2 = 852;
            tone1 = 1477;
            break;
        case '*':
            tone2 = 941;
            tone1 = 1209;
            break;
        case '#':
            tone2 = 941;
            tone1 = 1477;
            break;
        case 'A':
            tone2 = 697;
            tone1 = 1633;
            break;
        case 'B':
            tone2 = 770;
            tone1 = 1633;
            break;
        case 'C':
            tone2 = 852;
            tone1 = 1633;
            break;
        case 'D':
            tone2 = 941;
            tone1 = 1633;
            break;
        }
        if(tone1)
        {
            modem_snd_play_digit(handle, buf, bufLen, tone1, tone2);
            modem_snd_play_random(handle, buf, bufLen / 3, 3);
        }   
        number++;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
//
// int modem_snd(char * number)
//
int modem_snd(char * number)
{
    int err;
    snd_pcm_t *handle;
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        misc_print(0, "ERROR: modem_snd() --> Playback open error: %s\n", snd_strerror(err));
        return -1;
    }
    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_U8,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  1,
                                  RATE,
                                  1,
                                  RATE * 30)) < 0)
    {
        misc_print(0, "ERROR: modem_snd() --> Playback open error: %s\n", snd_strerror(err));
        return -2;
    }
    snd_pcm_nonblock(handle, 0);
    if(number != NULL)
    {
        if(number[0] == 'C' && number[1] == (char) 0x00)
        {
            char buf[RATE / 8 * 30];
            modem_snd_play_random(handle, buf, sizeof(buf), CONNECT_VOLUME);
        }
        else if(number[0] == 'R')
        {
            char buf[RATE / 8 * 5];
            modem_snd_play_random(handle, buf, sizeof(buf), 7);
            modem_snd_play_digit(handle, buf, sizeof(buf), 600, 650);
            modem_snd_play_random(handle, buf, sizeof(buf), 7);
            modem_snd_play_digit(handle, buf, sizeof(buf), 600, 650);
            modem_snd_play_random(handle, buf, sizeof(buf), 7);
        }
        else
        {
            char buf[RATE / 8 * 10];
            modem_snd_play_digit(handle, buf, sizeof(buf), 350, 440);
            modem_snd_play_number(handle, buf, RATE / 8, number);
        }
    }
    snd_pcm_drain(handle);
    //sleep(1);
    snd_pcm_close(handle);
    return TRUE;
}

