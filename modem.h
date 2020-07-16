void   modem_play_connect_sound(char * tmp);
void   modem_play_ring_sound(char * tmp);
void   modem_play_dial_sound(char * tmp, char * ipAddr);
void   modem_do_check_hangup(int * socket, char * buf, int bufLen);
void   modem_do_telnet_negotiate();
int    modem_do_file_picker(char * pathBuf, char * fileNameBuf);
void   modem_do_emulation(char * buf, int bufLen);
int    modem_handle_at_command(char * lineBuf);
void   modem_killall_aplay(int delay);
void   modem_killall_aplaymidi(int delay);
void   modem_killall_mpg123(int delay);
void * modem_tcplst_thread_function (void * x);
void * modem_tcpsock_thread_function (void * x);
int    modem_get_softsynth_port(int softSynth);
void   modem_set_defaults();



