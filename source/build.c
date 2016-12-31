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

    return ob;
}



static int create_six_plus_5(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating six-plus-5...\n");
    int err = run_create_six_plus_5(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(me) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 1, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", err);
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



static int create_six_plus_7(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating six-plus-7...\n");
    int err = run_create_six_plus_7(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(me) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 0, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", err);
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



static int create_texas_5(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating texas-5...\n");
    int err = run_create_texas_5(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(me) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 1, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", err);
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



static int create_test(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating test...\n");
    int err = run_create_test(ob);
    if (err != 0) {
        fprintf(stderr, "run_create_test(me) failed with %d as error code.\n", err);
        free_ofsm_builder(ob);
        return 1;
    }

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 1, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", err);
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



static int create_omaha_7(void)
{
    fprintf(stderr, "Not implemented create_omaha_7();\n");
    return 1;
}



typedef int (*create_func)(void);

struct poker_table
{
    const char * name;
    create_func create;
    create_func check;
};

static int stub(void)
{
    return 0;
}

struct poker_table poker_tables[] = {
    { "six-plus-5", create_six_plus_5, check_six_plus_5 },
    { "six-plus-7", create_six_plus_7, check_six_plus_7 },
    { "texas-5", create_texas_5, check_texas_5 },
    { "omaha-7", create_omaha_7, stub },
    #ifdef DEBUG_MODE
        { "test", create_test, check_test },
    #endif
    { NULL, NULL, NULL }
};



static void print_table_names(void)
{
    const struct poker_table * entry = poker_tables;
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

    int qerrors = check_opencl();

    const int qcalls = argc - first_arg;
    create_func calls[qcalls];
    for (int i=0; i<qcalls; ++i) {
        const char * const table_name = argv[first_arg + i];
        const struct poker_table * entry = poker_tables;
        for (;; ++entry) {
            if (entry->name == NULL) {
                ++qerrors;
                fprintf(stderr, "Table name “%s” is not found.\n", table_name);
                break;
            }
            if (strcmp(table_name, entry->name) == 0) {
                calls[i] = opt_check ? entry->check : entry->create;
                break;
            }
        }
    }

    if (qerrors > 0) {
        return 1;
    }

    int exit_code = 0;
    for (int j=0; j<qcalls; ++j) {
        int err = calls[j]();
        if (err != 0) {
            exit_code = 1;
            break;
        }
    }

    global_free();
    free_opencl();
    return exit_code;
}
