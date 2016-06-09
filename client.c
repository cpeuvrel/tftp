#include "utils.h"

int main(int argc, const char *argv[])
{
    int no_ext = 0; // Flag to show if can use RFC2347 extensions (0 = can use extension, 1 = no extension)

    size_t buffer_size = DEFAULT_BLK_SIZE; // Default buffer size until renegociated
    size_t pref_buffer_size = PREF_BLK_SIZE; // Block size going to be negociate

    struct conn_info conn; // Struct in which we will put all connection infos

    char host[HOST_LEN] = DST_HOST; // Destination's address
    char **filenames; // Array of all files
    char *buffer;
    size_t timeout = DEFAULT_TIMEOUT;
    int i;

    buffer=malloc(buffer_size * sizeof(char));

    filenames=malloc(argc * sizeof(char*));
    bzero(filenames, argc * sizeof(char*));

    // Parsing CLI
    opts(argc, argv, &pref_buffer_size, &timeout, &no_ext, host, HOST_LEN, filenames);

    if (filenames[0] == NULL)
        error("No file asked");

    for (i = 0; filenames[i] != NULL ; i++) {
        init_conn(&conn, host);

        bzero(buffer, buffer_size * sizeof(char));

        fprintf(stderr, "Downloading: %s\n", filenames[i]);
        if(send_rrq(conn, buffer, buffer_size, filenames[i], "octet", pref_buffer_size, timeout, no_ext) < 0)
            error("send_rrq");

        if(get_data(conn, &buffer, buffer_size, filenames[i]) < 0)
            error("get_data");

        free_conn(conn);
    }

    free (filenames);
    free (buffer);

    return 0;
}
