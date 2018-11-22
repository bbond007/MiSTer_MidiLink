#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/soundcard.h>
#include "misc.h"
#define USE_UDP
static struct sockaddr_in serv_addr;

///////////////////////////////////////////////////////////////////////////////////////
//
// int socket_client_connect(char * ipAddr, int port)
//
int socket_client_connect(char * ipAddr, int port)
{
    int sock = 0, valread;
#ifdef USE_UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
#else
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
#endif
    {
        printf("ERROR:socket_client_connect() --> Socket creation error\n");
        return -1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(strlen(ipAddr) == 0)
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if(inet_pton(AF_INET, ipAddr, &serv_addr.sin_addr)<=0)
    {
        printf("ERROR: socket_client_connect() --> Invalid IP address\n");
        return -1;
    }
#ifdef USE_UDP
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
#else
    printf("CONNECT : %s port %d\n", ipAddr, port);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR: socket_client_connect() --> Connection Failed\n");
        return -3;
    }

    printf("Connected to %s port %d\n", ipAddr, port);
#endif
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int socket_client_connect(char * ipAddr, int port)
//
void socket_write(int sock, char * buf, int bufLen)
{
#ifdef USE_UDP
    sendto(sock,
           (const char *) buf,
           bufLen,
           MSG_CONFIRM,
           (const struct sockaddr *) &serv_addr,
           sizeof(serv_addr));
#else
    send(sock, buf, bufLen, 0);
#endif
}

