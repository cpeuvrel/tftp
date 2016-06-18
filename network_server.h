#ifndef NETWORK_SERVER_H

#define NETWORK_SERVER_H

#include "network.h"

int init_server_conn(int server_port);
int send_oack(struct conn_info conn, char **opts, int *optval);
int handle_rq(struct conn_info conn, char **buffer, int n, int *buffer_size, FILE **fd_dst, enum request_code *type, int *last_block);
int rcv_data(int fd);

#endif /* end of include guard: NETWORK_SERVER_H */
