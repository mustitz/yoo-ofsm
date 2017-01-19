#include "yoo-ofsm.h"

#include <errno.h>
#include <string.h>

#define MAX_PTR_TO_FREE 20

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

    int errcode = ofsm_array_save_binary(array, f, name);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_array_save_binary_array(&array, f, “%s”) failed with %d as error code.\n", name, errcode);
    }

    if (ferror(f)) {
        fprintf(stderr, "I/O during saving binary file, ferror(f) returns nonzero.\n");
    }

    fclose(f);
}

static void * ptrs_to_free[MAX_PTR_TO_FREE];
static int qptr_to_free = 0;

void * global_malloc(size_t sz)
{
    void * result = malloc(sz);
    if (result == NULL) {
        return NULL;
    }

    if (qptr_to_free < MAX_PTR_TO_FREE) {
        ptrs_to_free[qptr_to_free++] = result;
    } else {
        fprintf(stderr, "Warning: ptrs_to_free overflow, please increase MAX_PTR_TO_FREE define.\n");
        fprintf(stderr, "Warning: Current value of MAX_PTR_TO_FREE define is %d.\n", MAX_PTR_TO_FREE);
    }

    return result;
}

void global_free(void)
{
    for (int i=0; i<qptr_to_free; ++i) {
        free(ptrs_to_free[i]);
    }
}
