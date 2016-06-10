#ifndef NETWORK_CLIENT_H

#define NETWORK_CLIENT_H

#include "network.h"

int send_rrq(struct conn_info conn, enum request_code type, char* buffer, int buffer_size, char* filename, char* mode, size_t pref_buffer_size, size_t timeout, int no_ext);
void handle_oack_c(struct conn_info conn, char **buffer, int *buffer_size, int n, int *final_size, char* filename);

#endif /* end of include guard: NETWORK_CLIENT_H */
