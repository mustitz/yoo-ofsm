#include "ofsm.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

void save_binary(const char * file_name, const char * name, const struct ofsm_array * array)
{
    const char * mode = "wb";
    FILE * f = fopen(file_name, mode);
    if (f == NULL) {
        fprintf(stderr, "fopen(“%s”, “%s”) failed with NULL as return value\n", file_name, mode);
        if (errno != 0) {
            fprintf(stderr, "  errno = %d: %s\n", errno, strerror(errno));
        }
    }

    int errcode = ofsm_save_binary_array(f, name, array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_save_binary_array(f, &Array) failed with %d as error code.\n", errcode);
    }

    if (ferror(f)) {
        fprintf(stderr, "I/O during saving binary file, ferror(f) returns nonzero.\n");
    }

    fclose(f);
}
