#include "network_server.h"

/* Create server's socket
 * Args:
 *  - server_port: Port to bind
 * Return:
 *  - socket's file descriptor
 *  */
int init_server_conn(int server_port)
{
    int enable = 1;

    struct sockaddr_in serv; // sockaddr for source
    int fd; // Socket's file descriptor
    int addr_len; // Address' size

    struct timeval tv;
    tv.tv_sec = DEFAULT_TIMEOUT; // Timeout in seconds
    tv.tv_usec = 0; // Timeout in microseconds

    addr_len=sizeof(serv);

    // Init socket
    if((fd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
        error("socket");

    // Allow it to be reuseable immediatly after end of use
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            error("setsockopt(reuse addr) failed");

    //set timer for recv_socket
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        error("setsockopt(rcv timeout) failed");

    // init serv
    bzero(&serv, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(server_port);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr*) &serv, addr_len))
        error("bind");

    return fd;
}
