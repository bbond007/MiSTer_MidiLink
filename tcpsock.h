int    tcpsock_client_connect(char * ipAddr, int port, int fdSerial); 
int    tcpsock_write(int sock, char * buf, int bufLen);
int    tcpsock_server_open(int port);
int    tcpsock_read(int sock, char * buf, int bufLen);
//void * socket_thread_function (void * x);
