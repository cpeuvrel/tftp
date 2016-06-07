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
#define PORT_MIN 10000 // Minimum port used as TID (source)
#define PORT_MAX 50000 // Maximum port used as TID (source)

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

/* Send an ERROR TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
 *  - err_code: Error code
 *  - err_msg: Error message
 *  */
void send_error(struct conn_info conn, int err_code, char *err_msg)
{
    char *buffer;

    buffer = malloc((4 + strlen(err_msg) + 1) * sizeof(char));

    buffer[0] = 0;
    buffer[1] = 5;
    buffer[2] = err_code / 256;
    buffer[3] = err_code % 256;

    sprintf(buffer+4, "%s", err_msg);

    if(sendto(conn.fd, buffer, 4, 0, conn.sock, conn.addr_len) < 0) {
        free(buffer);
        error("send_error");
    }

    free(buffer);
}

/* Send a ACK (ACKnowledgement) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
 *  - block_nb: Block# being acknowledged
 *  */
void send_ack(struct conn_info conn, int block_nb)
{
    char buffer[4];

    buffer[0] = 0;
    buffer[1] = 4;
    buffer[2] = block_nb / 256;
    buffer[3] = block_nb % 256;

    if(sendto(conn.fd, buffer, 4, 0, conn.sock, conn.addr_len) < 0)
        error("send_ack");
}

/* Handle DATA datagram (either client or server)
 * Args:
 *  - conn: Connections info to be able to send back ACK/ERROR
 *  - buffer: Buffer with the data received
 *  - n: Number of bytes in the buffer
 *  - last_block: Value pointed to is the block# from the last OK DATA
 *  - fd_dst: file descriptor to the file in which we write the data
 *  */
int handle_data(struct conn_info conn, char* buffer, int n, int *last_block, FILE *fd_dst)
{
    int block_nb; // Current block#

    // Minus opcode and block number
    n -= 4;

    block_nb = (unsigned char) buffer[2] * 256 + (unsigned char) buffer[3];

    if (block_nb != *last_block + 1)
        return -1;

    (*last_block)++;

    if ((int) fwrite(buffer+4, sizeof(char), n, fd_dst) != n) {
        send_error(conn, 3, "Disk full");
        error("Cannot write/disk full");
    }

    send_ack(conn, block_nb);

    return 0;
}

/* Loop in which we handle all data received for our request
 * Args:
 *  - conn: Connections info to be able to send back ACK/ERROR
 *  - buffer: Buffer with the data received
 *  - buffer_size: Maximum buffer size
 *  - filename: File we work on
 *  */
int get_data(struct conn_info conn, char **buffer, int buffer_size, char *filename)
{
    int end = 0; // Flag wether or not we can continue the loop
    int last_block = 0; // Block# of the last OK DATA
    int n; // Size of the last datagram we got
    FILE *fd_dst;

    // Remove file before trying to write to it
    unlink (filename);

    if ((fd_dst = fopen (filename, "ab")) == NULL)
        error("Open result file");

    bzero(*buffer, buffer_size);
    while ((n = recvfrom(conn.fd, *buffer, buffer_size, 0, conn.sock, (socklen_t *) &(conn.addr_len))) >= 0) {
        if (n < 0) {
            fprintf(stderr, "Timeout or error while receiving data\n");
            break;
        }

        if ((*buffer)[0] == 0) {
            switch ((*buffer)[1]) {
                case 3:
                    // DATA
                    if (handle_data(conn, *buffer, n, &last_block, fd_dst) == 0 &&
                            n < buffer_size - 4) {
                        end = 1;
                        break;
                    }
                    break;
                default:
                    // Anything else is an error (RRQ/WRQ/ACK/ERROR or non specified)
                    end = 1;
                    break;
            }
        }

        bzero(*buffer, buffer_size);

        if (end == 1)
            break;
    }


    fclose(fd_dst);

    return 0;
}

/* Init socket for the connection
 * Args:
 *  - conn: Connections info to set
 *  - host: Host to request
 *  */
void init_conn(struct conn_info *conn, char *host)
{
    int src_port; // Source port

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

    if(get_data(conn, &buffer, buffer_size, filenames[i]) < 0)
        error("get_data");

    free_conn(conn);
    free (buffer);

    return 0;
}
