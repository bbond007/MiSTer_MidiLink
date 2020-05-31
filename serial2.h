int  serial2_set_baud(char * serialDevice, int fdSerial, int baud);
int  serial2_set_baud_31250(char * serialDevice);
int  serial2_is_valid_rate (int baud);
void serial2_show_menu(int fdSerial);
int  serial2_baud_at_index(int index);
int  serial2_indexof(int baud);
int  serial2_set_DTR(int fd, int on);
int  serial2_get_DSR(int fd);
int  serial2_set_DCD(char * serialDevice, int fd, int on);

