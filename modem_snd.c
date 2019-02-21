#include <alsa/asoundlib.h>
#include <math.h>
#include <ctype.h>
#include "misc.h"

#define RATE 8000
#define VOLUME_DIALTONE  40
#define VOLUME_DIAL      70
#define VOLUME_CONNECT   100
#define VOLUME_RING      80
#define VOLUME_HISS1     3
#define VOLUME_HISS2     7  

///////////////////////////////////////////////////////////////////////////////////////
//int modem_snd_play_tones(snd_pcm_t *handle,
//                         char * buf,
//                         int bufLen,
//                         int tone1,
//                         int tone2,
//                         unsigned char max)

int modem_snd_play_tones(snd_pcm_t *handle,
                         char * buf,
                         int bufLen,
                         int tone1,
                         int tone2,
                         unsigned char max)
{
    int err;
    snd_pcm_sframes_t frames;
    double cost1 = 2.0 * M_PI * tone1 / (double) RATE;
    double cost2 = 2.0 * M_PI * tone2 / (double) RATE;
    for (int i = 0; i < bufLen ; i++)
    {
        double p1 = sin (i * cost1);
        double p2 = sin (i * cost2);
        unsigned char t1 = (p1 * max);
        unsigned char t2 = (p2 * max);
        buf[i] = t1 | t2;
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
//                          int bufLen,
//                          unsigned char)
int modem_snd_play_random(snd_pcm_t *handle,
                          char * buf,
                          int bufLen,
                          unsigned char max)
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
            tone1 = 1366;
            tone2 = 941;
            break;
        case '1':
            tone1 = 1209;
            tone2 = 697;
            break;
        case '2':
            tone1 = 1366;
            tone2 = 697;
            break;
        case '3':
            tone1 = 1477;
            tone2 = 697;
            break;
        case '4':
            tone1 = 1209;
            tone2 = 770;
            break;
        case '5':
            tone1 = 1366;
            tone2 = 770;
            break;
        case '6':
            tone1 = 1477;
            tone2 = 770;
            break;
        case '7':
            tone1 = 1209;
            tone2 = 852;
            break;
        case '8':
            tone1 = 1366;
            tone2 = 852;
            break;
        case '9':
            tone1 = 1477;
            tone2 = 852;
            break;
        case '*':
            tone1 = 1209;
            tone2 = 941;
            break;
        case '#':
            tone1 = 1477;
            tone2 = 941;
            break;
        case 'A':
            tone1 = 1633;
            tone2 = 697;
            break;
        case 'B':
            tone1 = 1633;
            tone2 = 770;
            break;
        case 'C':
            tone1 = 1633;
            tone2 = 852;
            break;
        case 'D':
            tone1 = 1633;
            tone2 = 941;
            break;
        case '.':
            tone1 = 1477;
            tone2 = 941;
            break;
        }
        if(tone1)
        {
            modem_snd_play_tones(handle, buf, bufLen, tone1, tone2, VOLUME_DIAL);
            modem_snd_play_random(handle, buf, bufLen / 3, VOLUME_HISS1);
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
            modem_snd_play_random(handle, buf, sizeof(buf), VOLUME_CONNECT);
        }
        else if(number[0] == 'R')
        {
            char buf[RATE / 8 * 5];
            modem_snd_play_random(handle, buf, sizeof(buf), VOLUME_HISS2);
            modem_snd_play_tones(handle,  buf, sizeof(buf), 600, 650, VOLUME_RING);
            modem_snd_play_random(handle, buf, sizeof(buf), VOLUME_HISS2);
            modem_snd_play_tones(handle,  buf, sizeof(buf), 600, 650, VOLUME_RING);
            modem_snd_play_random(handle, buf, sizeof(buf), VOLUME_HISS2);
        }
        else
        {
            char buf[RATE / 8 * 10];
            modem_snd_play_tones(handle,  buf, sizeof(buf), 350, 440, VOLUME_DIALTONE);
            modem_snd_play_number(handle, buf, RATE / 8, number);
        }
    }
    snd_pcm_drain(handle);
    //sleep(1);
    snd_pcm_close(handle);
    return TRUE;
}

