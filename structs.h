#ifndef CONN_INFO_H

#define CONN_INFO_H

struct conn_info {
    int fd; // File descriptor of the connection's socket
    struct sockaddr *sock; // Connection's socket
    int addr_len; // Size of the address
    void *free; // Use for easy free
};

enum request_code {
    RRQ = 1,
    WRQ = 2
};

enum tftp_role {
    CLIENT,
    SERVER
};

#endif /* end of include guard: CONN_INFO_H */
