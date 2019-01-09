#include <alsa/asoundlib.h>     /* Interface to the ALSA system */
#include <unistd.h>             /* for sleep() function */
#include"misc.h"

static snd_seq_event_t ev;
static snd_midi_event_t* parser;
static snd_seq_t * seq;
static int portNo = -1;
static int devNo  = -1;

///////////////////////////////////////////////////////////////////////////////////////
//
// void alsa_reset_seq_event(snd_seq_event_t * ev)
//
void alsa_reset_seq_event(snd_seq_event_t * ev)
{
    snd_seq_ev_clear(ev);
    snd_seq_ev_set_direct(ev);
    snd_seq_ev_set_dest(ev, portNo, devNo);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void alsa_send_midi_raw(char * buf, int bufLen)
//
void alsa_send_midi_raw(char * buf, int bufLen)
{
    for (int i = 0; i < bufLen; i++)
    {
        int result  = snd_midi_event_encode_byte(parser, buf[i], &ev);
        if(result == TRUE)
        {
            snd_seq_event_output(seq, &ev);
            snd_seq_drain_output(seq);
            alsa_reset_seq_event(&ev);
            snd_midi_event_reset_encode(parser);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int alsa_open_seq(int _portNo, int _devNo)
//
int alsa_open_seq(int _portNo, int _devNo)
{
    portNo = _portNo;
    devNo  = _devNo;
    misc_print(0, "Opening ALSA address --> %d:%d\n", portNo, devNo);
    if(snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        misc_print(0, "ERROR: snd_seq_open(%d, %d)\n", portNo, devNo);
        return FALSE;
    }
    alsa_reset_seq_event(&ev);
    snd_midi_event_new(256, &parser);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int alsa_get_midi_port(char * descr)
//
int alsa_get_midi_port(char * descr)
{
    static snd_seq_t * seqTst;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    char * endPtr;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    if (snd_seq_open(&seqTst, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        misc_print(0, "ERROR: alsa_get_midi_port() --> snd_seq_open() failed\n", portNo, devNo);
        return -1;
    }
    while (snd_seq_query_next_client(seqTst, cinfo) >= 0)
    {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seqTst, pinfo) >= 0)
        {
            if(strstr(snd_seq_client_info_get_name(cinfo), descr))
            {
                snd_seq_close(seqTst);
                return snd_seq_client_info_get_client(cinfo);
            }
        }
    }
    snd_seq_close(seqTst);
    return -1;
}
