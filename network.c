#include "network.h"

/* Struct with all we need to send a datagram */
/* Send an ERROR TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
 *  - err_code: Error code
 *  - err_msg: Error message
 *  */
void send_error(struct conn_info conn, int err_code, char *err_msg)
{
    char *buffer;
    int n;

    buffer = malloc((4 + strlen(err_msg) + 1) * sizeof(char));

    buffer[0] = 0;
    buffer[1] = 5;
    buffer[2] = err_code / 256;
    buffer[3] = err_code % 256;

    n = sprintf(buffer+4, "%s", err_msg);

    if(sendto(conn.fd, buffer, 4+n, 0, conn.sock, conn.addr_len) < 0) {
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
 *  - total_size: Value pointed to is the incremental size we got in this transaction
 *  - fd_dst: file descriptor to the file in which we write the data
 *  */
int handle_data(struct conn_info conn, char* buffer, int n, int *last_block, int *total_size, FILE *fd_dst)
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

    *total_size += n;

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
    int total_size = 0; // Incremental size of the file we got so far
    int final_size = -1; // Total size of the file we're supposed to get
    int n; // Size of the last datagram we got
    int got_one = 0 ; // Do we get at least one reply
    FILE *fd_dst;



    bzero(*buffer, buffer_size);
    while ((n = recvfrom(conn.fd, *buffer, buffer_size, 0, conn.sock, (socklen_t *) &(conn.addr_len))) >= 0) {
        if (n < 0) {
            fprintf(stderr, "Timeout or error while receiving data\n");
            break;
        }
        if (got_one == 0) {
            // Remove file before trying to write to it
            unlink (filename);

            if ((fd_dst = fopen (filename, "ab")) == NULL)
                error("Cannot open result file");

            got_one = 1;
        }

        if ((*buffer)[0] == 0) {
            switch ((*buffer)[1]) {
                case 6:
                    // OACK (Option ACK)
                    handle_oack_c(conn, buffer, &buffer_size, n, &final_size, filename);
                    send_ack(conn, 0);
                    break;
                case 3:
                    // DATA
                    if (handle_data(conn, *buffer, n, &last_block, &total_size, fd_dst) == 0 &&
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

    // If we didn't get any answer from server
    if (got_one == 0)
        return -1;

    fclose(fd_dst);

    if (final_size != -1 && final_size != total_size) {
        fprintf(stderr, "Final size of '%s' is wrong. Got %dB instead of %dB\n", filename, total_size, final_size);
        return -1;
    }

    return 0;
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

/* Init socket for the connection
 * Args:
 *  - conn: Connections info to set
 *  - host: Host to request
 *  */
void init_conn(struct conn_info *conn, char *host)
{
    int src_port; // Source port
    int enable = 1;

    struct sockaddr_in *dst, src; // sockaddr for destination and source
    int *fd; // Socket's file descriptor
    int *addr_len; // Address' size

    struct timeval tv;
    tv.tv_sec = DEFAULT_TIMEOUT; // Timeout in seconds
    tv.tv_usec = 0; // Timeout in microseconds

    fd = malloc(sizeof(int));
    addr_len = malloc(sizeof(int));
    dst = malloc(sizeof(struct sockaddr_in));

    *addr_len=sizeof(*dst);

    // Init socket
    if((*fd = socket( AF_INET, SOCK_DGRAM, 0)) < 0)
        error("socket");

    // Allow it to be reuseable immediatly after end of use
    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            error("setsockopt(reuse addr) failed");

    //set timer for recv_socket
    if (setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        error("setsockopt(rcv timeout) failed");

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
