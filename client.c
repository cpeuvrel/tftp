#include "utils.h"

int main(int argc, const char *argv[])
{
    int no_ext = 0; // Flag to show if can use RFC2347 extensions (0 = can use extension, 1 = no extension)

    size_t buffer_size = DEFAULT_BLK_SIZE; // Default buffer size until renegociated
    size_t pref_buffer_size = PREF_BLK_SIZE; // Block size going to be negociate

    int server_port = DEFAULT_SERVER_PORT;

    struct conn_info conn; // Struct in which we will put all connection infos

    char host[HOST_LEN] =  ""; // Destination's address
    char **filenames; // Array of all files
    int retry = DEFAULT_RETRY; // Number of retries on errors
    char *buffer;
    size_t timeout = DEFAULT_TIMEOUT;
    int i;

    int server_fd; // Server's socket's file descriptor

    enum request_code type = RRQ;
    enum tftp_role role = CLIENT;

    buffer=malloc(buffer_size * sizeof(char));

    filenames=malloc(argc * sizeof(char*));
    bzero(filenames, argc * sizeof(char*));

    // Parsing CLI
    opts(argc, argv, &server_port, &pref_buffer_size, &timeout, &no_ext, &type, &retry, &role, host, HOST_LEN, filenames);

    if (role == CLIENT) {
        if (strlen(host) == 0)
            error("-H is mandatory for clients");

        if (filenames[0] == NULL)
            error("No file asked");

        for (i = 0; filenames[i] != NULL ; i++) {
            init_client_conn(&conn, host, server_port);

            bzero(buffer, buffer_size * sizeof(char));

            if (type == RRQ)
                fprintf(stderr, "Downloading: %s\n", filenames[i]);
            else
                fprintf(stderr, "Uploading: %s\n", filenames[i]);

            if(send_rq(conn, type, buffer, buffer_size, filenames[i], "octet", pref_buffer_size, timeout, no_ext) < 0)
                error("send_rq");

            if(get_data(conn, type, retry, &buffer, buffer_size, filenames[i]) < 0)
                error("get_data");

            free_conn(conn);
        }
    }
    else {
        server_fd = init_server_conn(server_port);

        while (1) {
            rcv_data(server_fd);
        }
    }

    free (filenames);
    free (buffer);

    return 0;
}
