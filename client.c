#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

#include <netdb.h>
#include <arpa/inet.h>

#define DEFAULT_BLK_SIZE 516 // Default value defined in RFC1350 is 512 of payload + 4 of headers
#define DST_PORT 69   // Server port defined in RFC1350

/* Struct with all we need to send a datagram */
struct conn_info {
    int fd; // File descriptor of the connection's socket
    struct sockaddr *sock; // Connection's socket
    int addr_len; // Size of the address
    void *free[3]; // Use for easy free
};

/* Wrapper for perror
 * Args:
 *  - msg: Message printed to have context
 *  */
void error(char *msg)
{
    perror(msg);
    exit(errno);
}

/* Send a RRQ (Read ReQuest) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
 *  - buffer: Buffer with the data received
 *  - buffer_size: Maximum buffer size (can be modified here)
 *  - filename: File we are requesting
 *  - mode: mode of the request ("asciinet", "octet")
 * Return:
 *  Size of the datagram sent, or
 *  -1: Buffer too small
 *  */
int send_rrq(struct conn_info conn, char* buffer, int buffer_size, char* filename, char* mode)
{
    int total_len; // Final length of the datagram (used to avoid buffer overflow)
    int filename_l, mode_l; // Length of the strings filename/mode
    int i;

    filename_l = strlen(filename);
    mode_l = strlen(mode);

    bzero(buffer, buffer_size);

    total_len = 2 + filename_l + 1 + mode_l + 1 + 7 + 1 + 4 + 1 + 5 + 1 + 1 + 1 ;

    if (total_len > buffer_size)
        return -1;

    // Op code is 01 for RRQ (Read ReQuest)
    buffer[1] = 1;

    i = 2;

    // The +1 is for the final \0
    i += 1 + sprintf(buffer+i, "%s", filename);

    i += 1 + sprintf(buffer+i, "%s", mode);

    if(sendto(conn.fd, buffer, i, 0, conn.sock, conn.addr_len) < 0)
        error("send_rrq");

    return i;
}

/* Init socket for the connection
 * Args:
 *  - conn: Connections info to set
 *  - host: Host to request
 *  */
void init_conn(struct conn_info *conn, char *host)
{
    int src_port = 42042; // Source port

    struct sockaddr_in *dst, src; // sockaddr for destination and source
    int *fd; // Socket's file descriptor
    int *addr_len; // Address' size

    fd = malloc(sizeof(int));
    addr_len = malloc(sizeof(int));
    dst = malloc(sizeof(struct sockaddr_in));

    *addr_len=sizeof(*dst);

    // Init socket
    if((*fd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
        error("socket");

    // init Dest
    bzero(dst, sizeof(*dst));
    dst->sin_family = AF_INET;
    dst->sin_port = htons(DST_PORT);
    dst->sin_addr.s_addr = inet_addr(host);

    // init src
    bzero(&src, sizeof(src));
    src.sin_family = AF_INET;
    src.sin_port = htons(src_port);
    src.sin_addr.s_addr = htonl(INADDR_ANY);

    // Init struct conn_info
    bzero(conn, sizeof(*conn));
    conn->fd = *fd;
    conn->sock = (struct sockaddr*) dst;
    conn->addr_len = *addr_len;
    conn->free[0] = fd;
    conn->free[1] = dst;
    conn->free[2] = addr_len;

    if (bind(*fd, (struct sockaddr*) &src, *addr_len))
        error("bind");
}

/* Free the content of a struct conn_info
 * Args:
 *  - conn: The struct to free
 *  */
void free_conn(struct conn_info conn)
{
    free(conn.free[0]);
    free(conn.free[1]);
    free(conn.free[2]);
}

int main(int argc, const char *argv[])
{
    size_t buffer_size = DEFAULT_BLK_SIZE; // Default buffer size until renegociated

    struct conn_info conn; // Struct in which we will put all connection infos

    char host = "127.0.0.1"; // Destination's address
    char *filename = "foobar";
    char *buffer;

    buffer=malloc(buffer_size * sizeof(char));

    init_conn(&conn, host);

    bzero(buffer, buffer_size * sizeof(char));

    if(send_rrq(conn, buffer, buffer_size, filename, "octet", pref_buffer_size, timeout, no_ext) < 0)
        error("send_rrq");

    free_conn(conn);
    free (buffer);

    return 0;
}
