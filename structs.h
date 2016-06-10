#ifndef CONN_INFO_H

#define CONN_INFO_H

struct conn_info {
    int fd; // File descriptor of the connection's socket
    struct sockaddr *sock; // Connection's socket
    int addr_len; // Size of the address
    void *free[3]; // Use for easy free
};

#endif /* end of include guard: CONN_INFO_H */
