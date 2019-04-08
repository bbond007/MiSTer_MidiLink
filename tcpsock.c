#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/soundcard.h>
#include <errno.h>
#include "misc.h"
static struct sockaddr_in server_addr;
extern int MIDI_DEBUG;


///////////////////////////////////////////////////////////////////////////////////////
//
// int tcpsock_set_tcp_nodelay(int sock)
//
int tcpsock_set_tcp_nodelay(int sock)
{
    int nodelay_flag = 1;
    misc_swrite(1, "Setting --> TCP_NODELAY\n");
    if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*) &nodelay_flag, sizeof(int)) < 0)
    {
            misc_swrite(1, "ERROR: tcpsock_set_tcp_nodelay() -->  %s\n", strerror(errno));
    }
}
    
///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_client_connect(char * ipAddr, int port)
//
int tcpsock_client_connect(char * ipAddr, int port, int fdSerial)
{
    char tmp[100] = "";
    int sock = 0, valread;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        if(fdSerial > 0)
            misc_swrite(fdSerial, "\r\nERROR: tcpsock_client_connect() --> Socket creation error : %s", strerror(errno));
        return -1;
    }
       
    tcpsock_set_tcp_nodelay(sock);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAddr, &server_addr.sin_addr) <= 0)
    {
        if (fdSerial > 0)
            misc_swrite(fdSerial, "\r\nERROR: tcpsock_client_connect() --> Invalid IP address : '%s'", ipAddr);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        if (fdSerial > 0)
            misc_swrite(fdSerial,"\r\nERROR: tcpsock_client_connect() --> %s", strerror(errno));
        return -1;
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int tcpsock_write(int sock, char * ipAddr, int port)
//
int tcpsock_write(int sock, char * buf, int bufLen)
{
    int result = write(sock, buf, bufLen);
    if (result < 0)
        misc_print(1, "ERROR: tcpsock_write() --> %d : %s\n", result, strerror(errno));
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int tcpsock_server_open(int port)
//
int tcpsock_server_open(int port)
{
    int sock;
    struct sockaddr_in servaddr;
    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        misc_print(1, "ERROR: socket_server_open() --> socket creation failed");
        return -1;
    }
    
    tcpsock_set_tcp_nodelay(sock);

    memset(&servaddr, 0, sizeof(servaddr));
    // Filling server information
    servaddr.sin_family      = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port 	     = htons(port);
    // Bind the socket with the server address
    if (bind(sock, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0 )
    {
        misc_print(0, "ERROR: tcpsock_server_open() --> bind failed :%s\n",
                   strerror(errno));
        return -1;
    }
    listen(sock, 10);
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int tcpsock_read(int sock, char * buf, int bufLen)
//
int tcpsock_read(int sock, char * buf,  int bufLen)
{
    int rdLen;
    rdLen = read(sock, buf, bufLen);
        if (rdLen < 0)
            misc_print(1, "ERROR: tcpsock_read() --> %d : %s\n", rdLen, strerror(errno));
    return rdLen;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  int tcpsock_set_timeout(int sock, int timeSec);
//
int tcpsock_set_timeout(int sock, int timeSec)
{
    struct timeval tv;
    tv.tv_sec = timeSec;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////
//
//  int tcpsock_accept(int sock)
//
int tcpsock_accept(int sock)
{
    int socket = accept(sock, (struct sockaddr*)NULL, NULL);
    if (socket < 0)
        misc_print(1, "ERROR: tcpsock_accept(%d) --> %s\n", sock, strerror(errno));
    return socket;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  int tcpsock_getSO_ERROR(int sock)
//
int tcpsock_getSO_ERROR(int sock)
{
    int err = 1;
    socklen_t len = sizeof err;
    if (-1 == getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&err, &len))

    misc_print(1, "ERROR: tcpsock_getSO_RRROR(%d) --> %s\n", sock, strerror(errno));
    if (err)
        errno = err;              // set errno to the socket SO_ERROR
    return err;
}

///////////////////////////////////////////////////////////////////////////////////////
//
//  int tcpsock_close(int sock)
//
int tcpsock_close(int sock)
{
    if (sock >= 0)
    {
        tcpsock_getSO_ERROR(sock); // first clear any errors, which can cause close to fail
        if (shutdown(sock, SHUT_RDWR) < 0) // secondly, terminate the 'reliable' delivery
            if (errno != ENOTCONN && errno != EINVAL) // SGI causes EINVAL
                misc_print(1, "ERROR: tcpsock_close(%d) --> shutdown() : %s\n", sock, strerror(errno));

        if (close(sock) < 0) // finally call close()
            misc_print(1, "ERROR: tcpsock_close(%d) close() --> %s\n", sock, strerror(errno));
    }
    else
        misc_print(1, "ERROR: tcpsock_close() --> Invalid File Descriptor '%s'.\n", sock);
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void tcpsock_get_ip(int sock, char * buf) 
//
void tcpsock_get_ip(int sock, char * buf) 
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(sock, (struct sockaddr *)&addr, &addr_size);
    strcpy(buf, inet_ntoa(addr.sin_addr));
}