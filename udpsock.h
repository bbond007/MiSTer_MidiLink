int    udpsock_client_connect(char * ipAddr, int port); 
void   udpsock_write(int sock, char * buf, int bufLen);
int    udpsock_server_open(int port);
int    udpsock_read(int sock, char * buf, int bufLen);
//void * socket_thread_function (void * x);
