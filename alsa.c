#include <alsa/asoundlib.h>     /* Interface to the ALSA system */
#include <unistd.h>             /* for sleep() function */

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
        if(result == 1)
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
    printf ("Opening ALSA address --> %d:%d\n", portNo, devNo);
    if(snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        printf("ERROR: snd_seq_open(%d, %d)\n", portNo, devNo);
        return -1;
    }
    alsa_reset_seq_event(&ev);
    snd_midi_event_new(256, &parser);
    return 0;
}

