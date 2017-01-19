#include <errno.h>
#include <getopt.h>
#include <string.h>

#include "yoo-ofsm.h"
#include "poker.h"



#define MAX_PTR_TO_FREE 20



int opt_check = 0;
int opt_verbose = 0;
int opt_help = 0;
int opt_opencl = -1;

static void * ptrs_to_free[MAX_PTR_TO_FREE];
static int qptr_to_free = 0;



/* Utils */

static int create_ofsm(const struct poker_ofsm * const poker_ofsm)
{
    int status;

    struct ofsm_builder * restrict const ob = create_ofsm_builder(NULL, stderr);
    if (ob == NULL) {
        fprintf(stderr, "create_ofsm_builder(NULL, stderr) failed with NULL as result value.\n");
        return 1;
    }

    if (opt_verbose) {
        ob->logstream = stdout;
    }

    ob->flags |= OBF__AUTO_VERIFY;

    printf("Creating %s...\n", poker_ofsm->name);
    status = poker_ofsm->create(ob);
    if (status != 0) {
        fprintf(stderr, "poker_ofsm->create(ob) failed with %d as error code for %s.\n", status, poker_ofsm->signature);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code for %s.\n", poker_ofsm->delta, status, poker_ofsm->signature);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary(poker_ofsm->file_name, poker_ofsm->signature, &array);

    free(array.array);
    free_ofsm_builder(ob);
    return status;
}

void save_binary(const char * const file_name, const char * const name, const struct ofsm_array * const array)
{
    const char * const mode = "wb";
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

void * global_malloc(const size_t sz)
{
    void * const result = malloc(sz);
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



/* Parse command line */

static void usage(void)
{
    printf("%s",
        "USAGE: yoo-build-poker-ofsm [OPTION] table1 [table2 ... tableN]\n"
        "  --check, -c       Run poker table verification no generation.\n"
        "  --help, -h        Print usage and terminate.\n"
        "  --enable-opencl   Use OpenCL for verification.\n"
        "  --disable-opencl  Do not use OpenCL for verification.\n"
        "  --verbose, -v     Output an extended logging information to stderr.\n"
    );
}

static int parse_command_line(int argc, char * argv[])
{
    static const struct option long_options[] = {
        { "check", no_argument, &opt_check, 1 },
        { "help",  no_argument, &opt_help, 1},
        { "enable-opencl", no_argument, &opt_opencl, 1},
        { "disable-opencl", no_argument, &opt_opencl, 0},
        { "verbose", no_argument, &opt_verbose, 1 },
        { NULL, 0, NULL, 0 }
    };

    for (;;) {
        int index = 0;
        const int c = getopt_long(argc, argv, "hcv", long_options, &index);
        if (c == -1) break;

        if (c != 0) {
            switch (c) {
                case 'c':
                    opt_check = 1;
                    break;
                case 'h':
                    opt_help = 1;
                    break;
                case 'v':
                    opt_verbose = 1;
                    break;
                 case '?':
                    fprintf(stderr, "Invalid option.\n");
                    return -1;
                default:
                    fprintf(stderr, "getopt_long returns unexpected char \\x%02X.\n", c);
                    return -1;
            }
        }
    }

    return optind;
}



/* Poker table to generate/check */

#define POKER_OFSM(arg_name, arg_signature, arg_delta, arg_create, arg_check) { \
    .name = arg_name, \
    .file_name = arg_name ".bin", \
    .signature =arg_signature, \
    .delta = arg_delta, \
    .create = arg_create, \
    .check = arg_check }

const struct poker_ofsm poker_ofsms[] = {
    POKER_OFSM("six-plus-5", "OFSM Six Plus 5", 1, create_six_plus_5, check_six_plus_5),
    POKER_OFSM("six-plus-7", "OFSM Six Plus 7", 0, create_six_plus_7, check_six_plus_7),
    POKER_OFSM("texas-5", "OFSM Texas 5", 1, create_texas_5, check_texas_5),
    POKER_OFSM("texas-7", "OFSM Texas 7", 0, create_texas_7, check_texas_7),
    POKER_OFSM("omaha-7", "OFSM Omaha 7", 0, create_omaha_7, check_omaha_7),
    { NULL, NULL, NULL, 0, NULL, NULL }
};

static void print_table_names(void)
{
    const struct poker_ofsm * entry = poker_ofsms;
    for (;; ++entry) {
        if (entry->name == NULL) {
            break;
        }
        printf("%s\n", entry->name);
    }
}



#ifdef HAS_OPENCL
    static int check_opencl(void)
    {
        if (opt_opencl < 0) {
            opt_opencl = 1;
        }

        if (opt_opencl) {
            int status = init_opencl(stderr);
            if (status != 0) {
                fprintf(stderr, "OpenCL initialization error.\n");
                opt_opencl = 0;
                return 1;
            }
        }

        return 0;
    }
#else
    static int check_opencl(void)
    {
        if (opt_opencl == 1) {
            fprintf(stderr, "OpenCL is unsupported in the current build.\n");
            fprintf(stderr, "Please, run ./configure with option --enable-opencl and rebuild source to use OpenCL features.\n");
            return 1;
        }
        if (opt_opencl < 0) {
            opt_opencl = 0;
        }
        return 0;
    }
#endif



int main(int argc, char * argv[])
{
    const int first_arg = parse_command_line(argc, argv);

    if (first_arg < 0) {
        usage();
        return 1;
    }

    if (opt_help) {
        usage();
        return 0;
    }

    if (debug_something() != 0) {
        printf("Debug failed.\n");
        return 1;
    }

    if (first_arg >= argc) {
        print_table_names();
        return 0;
    }

    int qerrors = opt_check ? check_opencl() : 0;

    const int qcalls = argc - first_arg;
    const struct poker_ofsm * call_list[qcalls];
    for (int i=0; i<qcalls; ++i) {
        const char * const table_name = argv[first_arg + i];
        const struct poker_ofsm * entry = poker_ofsms;
        for (;; ++entry) {
            if (entry->name == NULL) {
                ++qerrors;
                fprintf(stderr, "Table name “%s” is not found.\n", table_name);
                break;
            }
            if (strcmp(table_name, entry->name) == 0) {
                call_list[i] = entry;
                break;
            }
        }
    }

    if (qerrors > 0) {
        return 1;
    }

    int exit_code = 0;
    if (opt_check) {
        for (int j=0; j<qcalls; ++j) {
            check_func call = call_list[j]->check;
            const int status = call();
            if (status != 0) {
                exit_code = 1;
                break;
            }
        }
    } else {
        for (int j=0; j<qcalls; ++j) {
            const int status = create_ofsm(call_list[j]);
            if (status != 0) {
                exit_code = 1;
                break;
            }
        }
    }

    global_free();
    free_opencl();
    return exit_code;
}
