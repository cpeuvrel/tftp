#include "utils.h"

/* Wrapper for perror
 * Args:
 *  - msg: Message printed to have context
 *  */
void error(char *msg)
{
    perror(msg);
    exit(errno);
}

/* Handle CLI arguments
 * Args:
 *  - argc: Number of CLI args
 *  - argv: Value of CLI args
 *  - pref_buffer_size: Buffer size going to be negociated
 *  - timeout: Timeout going to be negociated
 *  - no_ext: Flag to show if can use RFC2347 extensions (0 = can use extension, 1 = no extension)
 *  - host: Host to request
 *  - host_size: Max length of hostnames
 *  - filenames: Files we are requesting
 *  */
void opts(int argc, const char *argv[], size_t *pref_buffer_size, size_t *timeout, int *no_ext, char *host, size_t host_size, char **filenames)
{
    int i, choice, index; // Getopt stuff

    while ((choice = getopt(argc,(char * const*) argv, "H:b:t:e")) != -1) {

        switch( choice )
        {
            case 'H':
                if (strlen(optarg) > host_size)
                    error("Host too big");

                snprintf(host, host_size, "%s", optarg);
                break;

            case 'b':
                *pref_buffer_size = atoi(optarg);
                break;

            case 't':
                *timeout = atoi(optarg);
                break;

            case 'e':
                *no_ext = 1;
                break;

            default:
                exit(EXIT_FAILURE);
        }
    }

    /* Deal with non-option arguments here */
    for (i=0, index = optind; index < argc; i++, index++)
        filenames[i] = (char*) argv[index];
}
