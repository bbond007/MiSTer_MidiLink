int tcpsock_client_connect(char * ipAddr, int port, int fdSerial); 
int tcpsock_write(int sock, char * buf, int bufLen);
int tcpsock_server_open(int port);
int tcpsock_read(int sock, char * buf, int bufLen);
int tcpsock_set_timeout(int sock, int timeSec);
int tcpsock_close(int sock);
int tcpsock_accept(int sock);

