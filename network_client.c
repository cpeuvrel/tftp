#include "network_client.h"

/* Send a RRQ (Read ReQuest) TFTP datagram
 * Args:
 *  - conn: Connections info to be able to send the ACK
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
int send_rrq(struct conn_info conn, char* buffer, int buffer_size, char* filename, char* mode, size_t pref_buffer_size, size_t timeout, int no_ext)
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

    if (no_ext != 1) {
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
        error("send_rrq");

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
