//void alsa_reset_seq_event(snd_seq_event_t * ev);
void alsa_send_midi_raw(char * buf, int bufLen);
int  alsa_get_midi_port(char * descr);
int  alsa_open_seq(int _portNo, int _devNo);
void alsa_close_seq();

