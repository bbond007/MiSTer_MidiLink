#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/soundcard.h>
#include "misc.h"
static struct sockaddr_in server_addr;

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
        sprintf(tmp, "\r\nERROR: tcpsock_client_connect() --> Socket creation error\r\n\n");
        if (fdSerial > 0)
           write(fdSerial, tmp, strlen(tmp));
        return -1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (inet_pton(AF_INET, ipAddr, &server_addr.sin_addr) <= 0)
    {
        sprintf(tmp, "\r\nERROR: tcpsock_client_connect() --> Invalid IP address\r\n");
        if (fdSerial > 0)
           write(fdSerial, tmp, strlen(tmp));
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        sprintf(tmp,"\r\nERROR: tcpsock_client_connect()--> Connect Failed\r\n");
        if (fdSerial > 0)
           write(fdSerial, tmp, strlen(tmp));
        return -1;
    } 
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void tcpsock_write(int sock, char * ipAddr, int port)
//
void tcpsock_write(int sock, char * buf, int bufLen)
{
    write(sock, buf, bufLen); 
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
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        misc_print("ERROR: socket_server_open() --> socket creation failed");
        return -1;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    // Filling server information
    servaddr.sin_family      = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port 	     = htons(port);
    // Bind the socket with the server address
    if (bind(sock, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0 )
    {
        misc_print("ERROR: socket_server_open() --> bind failed\n");
        return -1;
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int tcpsock_read(int sock, char * buf, int bufLen)
//
extern int MIDI_DEBUG;

int tcpsock_read(int sock, char * buf,  int bufLen)
{
    int rdLen;
    rdLen = read(sock, buf, bufLen);
    return rdLen;
}
