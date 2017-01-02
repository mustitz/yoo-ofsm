#include <getopt.h>
#include <string.h>

#include "yoo-ofsm.h"
#include "poker.h"



int opt_check = 0;
int opt_verbose = 0;
int opt_help = 0;
int opt_opencl = -1;



static struct ofsm_builder * create_ob(void)
{
    struct ofsm_builder * restrict ob = create_ofsm_builder(NULL, stderr);
    if (ob == NULL) return NULL;

    if (opt_verbose) {
        ob->logstream = stdout;
    }

    ob->flags |= OBF__AUTO_VERIFY;
    return ob;
}



static int create_six_plus_5(const struct poker_ofsm * const poker_ofsm)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("Creating %s...\n", poker_ofsm->name);
    int err = run_create_six_plus_5(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_six_plus_5(ob) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code.\n", poker_ofsm->delta, err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("six-plus-5.bin", "OFSM Six Plus 5", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

static int check_six_plus_5(void)
{
    return run_check_six_plus_5();
}



static int create_six_plus_7(const struct poker_ofsm * const poker_ofsm)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("Creating %s...\n", poker_ofsm->name);
    int err = run_create_six_plus_7(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_six_plus_7(ob) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code.\n", poker_ofsm->delta, err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("six-plus-7.bin", "OFSM Six Plus 7", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

static int check_six_plus_7(void)
{
    return run_check_six_plus_7();
}



static int create_texas_5(const struct poker_ofsm * const poker_ofsm)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("Creating %s...\n", poker_ofsm->name);
    int err = run_create_texas_5(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(ob) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code.\n", poker_ofsm->delta, err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("texas-5.bin", "OFSM Texas 5", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

static int check_texas_5(void)
{
    return run_check_texas_5();
}



static int create_texas_7(const struct poker_ofsm * const poker_ofsm)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("Creating %s...\n", poker_ofsm->name);
    int err = run_create_texas_7(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_texas_7(ob) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code.\n", poker_ofsm->delta, err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("texas-7.bin", "OFSM Texas 7", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

static int check_texas_7(void)
{
    return run_check_texas_7();
}


static int create_test(const struct poker_ofsm * const poker_ofsm)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("Creating %s...\n", poker_ofsm->name);
    int err = run_create_test(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(ob) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, poker_ofsm->delta, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(ob, %d, &array) failed with %d as error code.\n", poker_ofsm->delta, err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("test.bin", "OFSM Test", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

static int check_test(void)
{
    return run_check_test();
}


static void usage(void)
{
    printf("%s",
        "USAGE: yoo-build-poker-ofsm [OPTION] table1 [table2 ... tableN]\n"
        "  --help, -h      Print usage and terminate.\n"
        "  --verbose, -v   Output an extended logging information to stderr.\n"
    );
}

static int parse_command_line(int argc, char * argv[])
{
    static struct option long_options[] = {
        { "enable-opencl", no_argument, &opt_opencl, 1},
        { "disable-opencl", no_argument, &opt_opencl, 0},
        { "check", no_argument, &opt_check, 1 },
        { "help",  no_argument, &opt_help, 1},
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



static int create_omaha_7(const struct poker_ofsm * const poker_ofsm)
{
    fprintf(stderr, "Not implemented create_omaha_7();\n");
    return 1;
}



static int stub(void)
{
    return 0;
}

#define POKER_OFSM(arg_name, arg_signature, arg_delta, arg_create, arg_check) { \
    .name = arg_name, \
    .filename = arg_name ".bin", \
    .signature =arg_signature, \
    .delta = arg_delta, \
    .create = arg_create, \
    .check = arg_check }

struct poker_ofsm poker_ofsms[] = {
    POKER_OFSM("six-plus-5", "OFSM Six Plus 5", 1, create_six_plus_5, check_six_plus_5),
    POKER_OFSM("six-plus-7", "OFSM Six Plus 7", 0, create_six_plus_7, check_six_plus_7),
    POKER_OFSM("texas-5", "OFSM Texas 5", 1, create_texas_5, check_texas_5),
    POKER_OFSM("texas-7", "OFSM Texas 7", 0, create_texas_7, check_texas_7),
    // POKER_OFSM("omaha-7", "OFSM Omaha 7", 0, create_omaha_7,check_omaha_7),
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
            int err = init_opencl(stderr);
            if (err != 0) {
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
            int err = call();
            if (err != 0) {
                exit_code = 1;
                break;
            }
        }
    } else {
        for (int j=0; j<qcalls; ++j) {
            create_func call = call_list[j]->create;
            int err = call(call_list[j]);
            if (err != 0) {
                exit_code = 1;
                break;
            }
        }
    }


    global_free();
    free_opencl();
    return exit_code;
}
