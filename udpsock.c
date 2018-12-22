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
static struct sockaddr_in serv_addr;

///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_client_connect(char * ipAddr, int port)
//
int udpsock_client_connect(char * ipAddr, int port)
{
    int sock = 0, valread;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("ERROR:socket_client_connect() --> Socket creation error\n");
        return -1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    //memset(&client_addr, 0, sizeof(client_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(strlen(ipAddr) == 0)
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if(inet_pton(AF_INET, ipAddr, &serv_addr.sin_addr)<=0)
    {
        printf("ERROR: socket_client_connect() --> Invalid IP address\n");
        return -1;
    }

    if (misc_ipaddr_is_multicast(ipAddr))
    {
        unsigned char multicastTTL = 1;
        printf("Enabling MULTICAST\n");
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) 
            &multicastTTL, sizeof(multicastTTL)) < 0)
        {
            printf("ERROR: socket_client_connect() --> setsockopt MULTICAST failed\n");
            return -2;
        }
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// void udpsock_write(char * ipAddr, int port)
//
void udpsock_write(int sock, char * buf, int bufLen)
{
    sendto(sock,
           (const char *) buf,
           bufLen,
           MSG_CONFIRM,
           (const struct sockaddr *) &serv_addr,
           sizeof(serv_addr));
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int uspsock_server_open(int port)
//
int udpsock_server_open(int port)
{
    int sock;
    struct sockaddr_in servaddr;
    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        printf("ERROR: socket_server_open() --> socket creation failed");
        return -1;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    //memset(&client_addr, 0, sizeof(client_addr));
    
    // Filling server information
    servaddr.sin_family      = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port 	     = htons(port);
    // Bind the socket with the server address
    if (bind(sock, (const struct sockaddr *)&servaddr,
              sizeof(servaddr)) < 0 )
    {
        printf("ERROR: socket_server_open() --> bind failed\n");
        return -2;
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_read(int sock, char * buf, int bufLen)
//
int udpsock_read(int sock, char * buf,  int bufLen)
{
    int rdLen, addrLen;
    struct sockaddr_in client_addr;
    addrLen = sizeof(client_addr);
    rdLen = recvfrom(sock, 
                     (char *) buf, 
                     bufLen,
                     MSG_WAITALL, 
                     ( struct sockaddr *) &client_addr,
                     &addrLen);
    if (rdLen < 0) 
        printf("ERROR : udpsock_read(%d, %d, %d) --> rdLen = %d\n", sock, buf, bufLen, rdLen);
    return rdLen;
}
