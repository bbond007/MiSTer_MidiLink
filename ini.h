int  ini_read_ini(char * fileName, char * section, int priority);
void ini_print_settings(int p);
void ini_bool (char * value, int          * dest);
void ini_int  (char * value, int          * dest);
void ini_uint (char * value, unsigned int * dest);
void ini_str  (char * key,   char * value,  char * dest, int max);
char ini_process_key_value_pair(char * key, char * value);
char ini_first_char(char * str, int len);
int  ini_parse_line(char * str, int len, char * key, int keymax, char * val, 
                    int valMax, char * sec, int secMax);
int  ini_read_loop (char * fileName, char * section, int prority, char * key, 
                    int keyMax, char * val, int valmax, char * sec, int secMax);
