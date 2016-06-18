#include "network_client.h"

/* Send a RRQ (Read ReQuest) or WRQ (Write ReQuest) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
 *  - type: Type of request (RRQ/WRQ)
 *  - buffer: Buffer with the data received
 *  - buffer_size: Maximum buffer size (can be modified here)
 *  - filename: File we are requesting
 *  - mode: mode of the request ("asciinet", "octet")
 *  - pref_buffer_size: Buffer size going to be negociated
 *  - timeout: Timeout going to be negociated
 *  - no_ext: Flag to show if can use RFC2347 extensions (0 = can use extension, 1 = no extension)
 * Return:
 *  Size of the datagram sent, or
 *  -1: Buffer too small
 *  */
int send_rq(struct conn_info conn, enum request_code type, char* buffer, int buffer_size, char* filename, char* mode, size_t pref_buffer_size, size_t timeout, int no_ext)
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

    // Op code
    buffer[1] = type;

    i = 2;

    // The +1 is for the final \0
    i += 1 + sprintf(buffer+i, "%s", filename);

    i += 1 + sprintf(buffer+i, "%s", mode);

    if (type == RRQ && no_ext != 1) {
        i += 1 + sprintf(buffer+i, "tsize");
        i += 1 + sprintf(buffer+i, "0");
    }

    if (pref_buffer_size != 0 && no_ext != 1) {
        i += 1 + sprintf(buffer+i, "blksize");
        i += 1 + sprintf(buffer+i, "%d", (int) pref_buffer_size);
    }

    if (timeout != 0 && no_ext != 1) {
        i += 1 + sprintf(buffer+i, "timeout");
        i += 1 + sprintf(buffer+i, "%d", (int) timeout);
    }

    if(sendto(conn.fd, buffer, i, 0, conn.sock, conn.addr_len) < 0)
        error("send_rq");

    return i;
}

/* Handle OACK (Option ACKnowledgement) datagram from a client perspective
 * Args:
 *  - conn: Connections info to be able to modify socket's timeout
 *  - buffer: Buffer with the data received
 *  - buffer_size: Maximum buffer size (can be modified here)
 *  - n: Number of bytes in the buffer
 *  - final_size: Value pointed to is the total size of the file we're supposed to get
 *  - filename: File we work on
 *  */
void handle_oack_c(struct conn_info conn, char **buffer, int *buffer_size, int n, int *final_size, char* filename)
{
    int i, timeout;
    struct timeval tv;

    for (i = 2; i < n; i++) {
        if (strncmp(*buffer+i, "blksize\0", 8) == 0) {
            i += 8;

            // Size asked + TFTP header
            *buffer_size = atoi(*buffer + i) + 4;

            *buffer = realloc (*buffer, *buffer_size * sizeof(char));
        }
        else if (strncmp(*buffer+i, "tsize\0", 6) == 0) {
            i += 6;

            *final_size = atoi(*buffer + i);
            fprintf(stderr, "Size of '%s': %d\n",filename, *final_size);
        }
        else if (strncmp(*buffer+i, "timeout\0", 8) == 0) {
            timeout = atoi(*buffer + i);

            tv.tv_sec = timeout; // Timeout in seconds
            tv.tv_usec = 0; // Timeout in microseconds

            //set timer for recv_socket
            if (setsockopt(conn.fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
                error("setsockopt(custom rcv timeout) failed");
        }

        // Consume last chars until next \0
        while ((*buffer)[i] != 0 && i < n)
            i++;
    }
}

/* Init socket for the connection
 * Args:
 *  - conn: Connections info to set
 *  - host: Host to request
 *  - server_port: Port to request
 *  */
void init_client_conn(struct conn_info *conn, char *host, int server_port)
{
    int src_port; // Source port
    int enable = 1;

    struct sockaddr_in *dst, src; // sockaddr for destination and source
    int fd; // Socket's file descriptor
    int addr_len; // Address' size

    struct timeval tv;
    tv.tv_sec = DEFAULT_TIMEOUT; // Timeout in seconds
    tv.tv_usec = 0; // Timeout in microseconds

    /*fd = malloc(sizeof(int));*/
    /*addr_len = malloc(sizeof(int));*/
    dst = malloc(sizeof(struct sockaddr_in));

    addr_len=sizeof(*dst);

    // Init socket
    if((fd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
        error("socket");

    // Allow it to be reuseable immediatly after end of use
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            error("setsockopt(reuse addr) failed");

    //set timer for recv_socket
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        error("setsockopt(rcv timeout) failed");

    // init Dest
    bzero(dst, sizeof(*dst));
    dst->sin_family = AF_INET;
    dst->sin_port = htons(server_port);
    dst->sin_addr.s_addr = inet_addr(host);

    // Generate a random source port between PORT_MIN and PORT_MAX
    srand(time(NULL));
    src_port = (rand() % (PORT_MAX - PORT_MIN)) + PORT_MIN;

    // init src
    bzero(&src, sizeof(src));
    src.sin_family = AF_INET;
    src.sin_port = htons(src_port);
    src.sin_addr.s_addr = htonl(INADDR_ANY);

    // Init struct conn_info
    bzero(conn, sizeof(*conn));
    conn->fd = fd;
    conn->sock = (struct sockaddr*) dst;
    conn->addr_len = addr_len;
    conn->free = dst;

    if (bind(fd, (struct sockaddr*) &src, addr_len))
        error("bind");
}
