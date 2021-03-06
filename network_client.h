#ifndef NETWORK_CLIENT_H

#define NETWORK_CLIENT_H

#include "network.h"

#include <sys/stat.h>

int send_rq(struct conn_info conn, enum request_code type, char* buffer, int buffer_size, char* filename, char* mode, size_t pref_buffer_size, size_t timeout, int no_ext);
void handle_oack_c(struct conn_info conn, char **buffer, int *buffer_size, int n, int *final_size, char* filename);

void init_client_conn(struct conn_info *conn, char *host, int server_port);

#endif /* end of include guard: NETWORK_CLIENT_H */
