#ifndef NETWORK_H

#define NETWORK_H

#include <sys/socket.h>
#include <arpa/inet.h>

#include "structs.h"
#include "utils.h"
#include "network_client.h"
#include "network_server.h"

#define DEFAULT_SERVER_PORT 69   // Server port defined in RFC1350
#define DEFAULT_BLK_SIZE 516 // Default value defined in RFC1350 is 512 of payload + 4 of headers
#define DEFAULT_TIMEOUT 1 // Default timeout is 1 second

#define PREF_BLK_SIZE 1468 // Maximum block size possible:
                      // Ethernet MTU (1500) - UDP headers (8) - IP (20)

#define HOST_LEN 128  // Maximum length of a hostname
#define PORT_MIN 10000 // Minimum port used as TID (source)
#define PORT_MAX 50000 // Maximum port used as TID (source)

#define DEFAULT_RETRY 3 // Number of retries on errors

void send_error(struct conn_info conn, int err_code, char *err_msg);
void send_ack(struct conn_info conn, int block_nb);
int send_data(struct conn_info conn, char** buffer, int buffer_size, int* last_block, FILE* fd);
int handle_data(struct conn_info conn, char* buffer, int n, int *last_block, int *total_size, FILE *fd_dst);
int handle_ack(char* buffer, int buffer_size, int last_block);
int get_data(struct conn_info conn, enum request_code type, const int retry, char **buffer, int buffer_size, char *filename);
void free_conn(struct conn_info conn);

#endif /* end of include guard: NETWORK_H */
