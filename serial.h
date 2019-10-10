int serial_set_interface_attribs(int fdSerial);
void serial_do_tcdrain(int fdSerial);
int serial_set_flow_control(int fdSerial, int hayesMode);
char * serial_hayes_flow_to_str(int hayesModem);
void serial_set_timeout(int fdSerial, int timeout);