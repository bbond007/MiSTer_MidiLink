int  misc_check_args_option (int argc, char *argv[], char * option);
void misc_str_to_upper(char *str);
int  misc_check_file (char * fileName);
int  misc_check_device (char * deviceName);
int  misc_set_priority(int priority);
int  misc_ipaddr_is_multicast(char * ipAddr);
void misc_print(int priority, const char* format, ... );
int  misc_is_ip_addr(char *ipAddr);
int  misc_hostname_to_ip(char * hostname , char* ipAddr);
int  misc_get_ipaddr(char * interface, char * buf);
int  misc_is_number(char *testStr);
void misc_show_atdt(int fdSerial);
void misc_show_atip(int fdSerial);
long misc_get_timeval_diff(struct timeval * start, struct timeval * stop);
int  misc_check_module_loaded (char * modName);
int  misc_get_midi_port(char * descr);
void misc_write_ok6(int fdSerial);
void misc_write_ok4(int fdSerial);
int  misc_list_files(char * path, int fdSerial, int rows, char * fileName, int * DIR);
void misc_d_type_to_str(unsigned char type, char * buf);
int  misc_file_to_serial(int fdSerial,  char * fileName, int rows);
int  misc_count_str_chr(char * str, char chr);
int  misc_MT32_LCD(char * MT32Message, char * buf);
void misc_show_at_commands(int fdSerial, int rows);
void misc_do_rowcheck(int fdSerial, int rows, int * rowcount, char * c, int CR);
int  misc_do_pipe(int fdSerial,  char * path, char * command, 
                  char * arg1, 
                  char * arg2,
                  char * arg3, 
                  char * arg4,
                  char * arg5);

#define TRUE 1
#define FALSE 0
