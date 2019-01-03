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
long misc_get_timeval_diff(struct timeval start, struct timeval stop);

#define TRUE 1
#define FALSE 0
