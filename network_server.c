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

/* Send a OACK (Option ACKnowledgement) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the OACK
 *  - opts: Options' name
 *  - optval: Options' values
 *  */
int send_oack(struct conn_info conn, char **opts, int *optval)
{
    // TODO
    return 0;
}

/* Handle RRQ/WRQ (Read/Write ReQuest) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send back the reply (OACK/ACK/DATA)
 *  - buffer: Buffer with the data received
 *  - n: Number of bytes received
 *  - buffer_size: Maximum size of the buffer
 *  - fd_dst: file descriptor to the file
 *  - type: Set if we are in RRQ or WRQ
 *  - last_block: Set the number of current data
 *  */
int handle_rq(struct conn_info conn, char **buffer, int n, int *buffer_size, FILE **fd_dst, enum request_code *type, int *last_block)
{
    // TODO
    return 0;
}

/* Main function of server. Accept a connection, and answer it
 * Args:
 *  - fd: Socket's file descriptor
 * */
int rcv_data(int fd)
{
    const int oretry = 3;

    struct conn_info conn;
    enum request_code type = NO;
    char *buffer;
    char *filename = NULL;

    int buffer_size = DEFAULT_BLK_SIZE;
    int end = 0; // Flag wether or not we can continue the loop
    int last_block = 0; // Block# of the last OK DATA
    int total_size = 0; // Incremental size of the file we got so far
    int final_size = -1; // Total size of the file we're supposed to get
    int n; // Size of the last datagram we got
    int wait_last_ack = 0 ; // Do we just wait for the last ACK (no more DATA to send)
    int first_dgram = 1 ;
    int retry = oretry ;
    FILE *fd_dst = NULL;

    buffer = malloc(sizeof(char) * buffer_size);

    struct sockaddr_in *dst;
    dst = malloc(sizeof(struct sockaddr_in));
    bzero(dst, sizeof(*dst));

    // Init struct conn_info
    bzero(&conn, sizeof(conn));
    conn.fd = fd;
    conn.sock = (struct sockaddr*) dst;
    conn.addr_len = sizeof(*dst);
    conn.free = dst;

    bzero(buffer, buffer_size);
    while ((n = recvfrom(fd, buffer, buffer_size, 0, conn.sock, (socklen_t *) &(conn.addr_len)))) {
        // On receive fail
        if (n < 0) {
            // If conn.sock wasn't initialize, ignore it
            if (((struct sockaddr_in*) conn.sock)->sin_port == 0)
                continue;

            retry--;

            // If we did all retries, break the main loop
            if (retry <= 0) {
                fprintf(stderr, "Receive %dB from %s:%d\n",n,
                        inet_ntoa(((struct sockaddr_in*) conn.sock)->sin_addr),
                        ntohs(((struct sockaddr_in*) conn.sock)->sin_port));
                fprintf(stderr, "FAIL\n");
                break;
            }

            continue;
        }


        if (first_dgram) {
            fprintf(stderr, "Receive %dB from %s:%d\n",n,
                    inet_ntoa(((struct sockaddr_in*) conn.sock)->sin_addr),
                    ntohs(((struct sockaddr_in*) conn.sock)->sin_port));
            first_dgram = 0;
        }

        // Reset retry for next failed receive
        retry = oretry;

        if (buffer[0] == 0) {
            switch (buffer[1]) {
                case 1:
                case 2:
                    handle_rq(conn, &buffer, n, &buffer_size, &fd_dst, &type, &last_block);
                    break;
                case 3:
                    // DATA
                    if (type == RRQ) {
                        send_error(conn, 4, "Illegal TFTP operation");
                        end = 1;
                        break;
                    }

                    if (handle_data(conn, buffer, n, &last_block, &total_size, fd_dst) == 0 &&
                            n < buffer_size - 4) {
                        end = 1;
                        break;
                    }
                    break;
                case 4:
                    // ACK
                    if (type == WRQ) {
                        send_error(conn, 4, "Illegal TFTP operation");
                        end = 1;
                        break;
                    }

                    if (handle_ack(buffer, n, last_block) == 0) {
                        if (wait_last_ack != 1) {
                            wait_last_ack = send_data(conn, &buffer, buffer_size, &last_block, fd_dst);
                        }
                        else {
                            end = 1;
                        }
                    }

                    break;
                case 5:
                    // ERROR
                    end = 1;
                    break;
                default:
                    // Anything else is an error (OACK or non specified)
                    send_error(conn, 4, "Illegal TFTP operation");
                    end = 1;
                    break;
            }
        }
        else {
            send_error(conn, 4, "Illegal TFTP operation");
            end = 1;
        }

        bzero(buffer, buffer_size);

        if (end == 1)
            break;
    }

    if (fd_dst != NULL)
        fclose(fd_dst);

    if (type == WRQ && final_size != -1 && final_size != total_size) {
        fprintf(stderr, "Final size of '%s' is wrong. Got %dB instead of %dB\n", filename, total_size, final_size);
        return -1;
    }

    return 0;
}
