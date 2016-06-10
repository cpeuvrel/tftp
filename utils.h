#ifndef UTILS_H

#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>

#include "network.h"

#define DST_HOST "192.168.0.100"

void error(char *msg);
void opts(int argc, const char *argv[], size_t *pref_buffer_size, size_t *timeout, int *no_ext, enum request_code *type, char *host, size_t host_size, char **filenames);

#endif /* end of include guard: UTILS_H */
