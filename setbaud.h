int  setbaud_set_baud(char * serialDevice, int fdSerial, int baud);
int  setbaud_set_baud_31250(char * serialDevice);
int  setbaud_is_valid_rate (int baud);
void setbaud_show_menu(int fdSerial);
int  setbaud_baud_at_index(int index);
int  setbaud_indexof(int baud);

