#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/soundcard.h>
#include <errno.h>
#include "misc.h"
static struct sockaddr_in server_addr;

///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_client_connect(char * ipAddr, int port)
//
int udpsock_client_connect(char * ipAddr, int port)
{
    int sock = 0, valread;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        misc_print(0, "ERROR:socket_client_connect() --> Socket creation error\n");
        return -1;
    }
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(strlen(ipAddr) == 0)
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    else if(inet_pton(AF_INET, ipAddr, &server_addr.sin_addr)<=0)
    {
        misc_print(0, "ERROR: udpsock_client_connect() --> Invalid IP address\n");
        return -1;
    }

    if (misc_ipaddr_is_multicast(ipAddr))
    {
        unsigned char multicastTTL = 1;
        misc_print(0, "Enabling MULTICAST\n");
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *)
                       &multicastTTL, sizeof(multicastTTL)) < 0)
        {
            misc_print(0, "ERROR: udpsock_client_connect() --> setsockopt MULTICAST failed\n");
            return -1;
        }
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_write(int sock, char * ipAddr, int port)
// 

int udpsock_write(int sock, char * buf, int bufLen)
{
    int result = sendto(sock,
                       (const char *) buf,
                       bufLen,
                       MSG_CONFIRM,
                       (const struct sockaddr *) &server_addr,
                       sizeof(server_addr));
    if (result < 0) 
         misc_print(1, "ERROR: udpsock_write() --> %d : %s\n", result, strerror(errno));
    return result;  
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
        misc_print(0, "ERROR: udpsock_server_open() --> socket creation failed");
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
        misc_print(0, "ERROR: udpsock_server_open() --> bind failed\n");
        return -1;
    }
    return sock;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// int udpsock_read(int sock, char * buf, int bufLen)
//
extern unsigned int UDPServerFilterIP;

int udpsock_read(int sock, char * buf,  int bufLen)
{
    int rdLen, addrLen;
    struct sockaddr_in client_addr;
    //memset(&client_addr, 0, sizeof(client_addr));
    addrLen = sizeof(client_addr);
    rdLen = recvfrom(sock,
                     (char *) buf,
                     bufLen,
                     MSG_WAITALL,
                     ( struct sockaddr *) &client_addr,
                     &addrLen);

    //throw out stuff not from the MIDI server
    if (UDPServerFilterIP && (client_addr.sin_addr.s_addr != server_addr.sin_addr.s_addr))
    {
        char server_str[20];
        char client_str[20];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_str, sizeof(client_addr));
        inet_ntop(AF_INET, &(server_addr.sin_addr), server_str, sizeof(server_addr));
        misc_print(2, "(client_addr --> %s) != (server_addr --> %s)\n", client_str, server_str);
        rdLen = 0;
    }
    if (rdLen < 0) 
        misc_print(1, "ERROR: tcpsock_read() --> %d : %s\n", rdLen, strerror(errno));
    return rdLen;
}
