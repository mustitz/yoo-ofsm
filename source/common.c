#include <ofsm.h>

#include <yoo-stdlib.h>

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>

#define input_t unsigned char
#define state_t uint32_t

#define STATUS__NEW           1
#define STATUS__EXECUTING     2
#define STATUS__FAILED        3
#define STATUS__INTERRUPTED   4
#define STATUS__DONE          5



static void errmsg(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void verbose(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));



#define ERRHEADER errmsg("Error in function “%s”, file %s:%d.", __FUNCTION__, __FILE__, __LINE__)

static void verrmsg(const char * fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static void errmsg(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verrmsg(fmt, args);
    va_end(args);
}



int opt_save_steps = 0;
int opt_verbose = 0;
int opt_help = 0;
FILE * verbose_stream = NULL;

void usage()
{
    printf("%s",
        "USAGE: yoo-ofsm [OPTIONS]\n"
        "  --help, -h             Print usage and terminate.\n"
        "  --save-steps, -s       Save OFSM after each step.\n"
        "  --verbose, -v          Output an extended logging information to stderr.\n"
    );
}

static int parse_command_line(int argc, char * argv[])
{
    static struct option long_options[] = {
        { "help",  no_argument, &opt_help, 1},
        { "save-steps", no_argument, &opt_save_steps, 1},
        { "verbose", no_argument, &opt_verbose, 1 },
        { NULL, 0, NULL, 0 }
    };

    for (;;) {
        int index = 0;
        int c = getopt_long(argc, argv, "ihsv", long_options, &index);
        if (c == -1) break;

        if (c != 0) {
            switch (c) {
                case 'h':
                    opt_help = 1;
                    break;
                case 's':
                    opt_save_steps = 1;
                    break;
                case 'v':
                    opt_verbose = 1;
                    break;
                case '?':
                    errmsg("Invalid option.");
                    return 1;
                default:
                    errmsg("getopt_long returns unexpected char \\x%02X.", c);
                    return 1;
            }
        }
    }

    if (opt_verbose) {
        verbose_stream = stderr;
    }

    if (optind < argc) {
        errmsg("Function “execute” do not expected any extra command line arguments except options.");
        verbose("optind = %d, argc = %d.", optind, argc);
        return 1;
    }


    return 0;
}



static void vverbose(const char * fmt, va_list args)
{
    if (verbose_stream == NULL) return;
    fprintf(verbose_stream, "[%12.6f] ", get_app_age());
    vfprintf(verbose_stream, fmt, args);
    fprintf(verbose_stream, "\n");
}

static void verbose(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vverbose(fmt, args);
    va_end(args);
}



struct flake
{
    input_t qinputs;
    state_t qstates;
    state_t * data;
};

struct ofsm
{
    unsigned int qflakes;
    unsigned int max_flakes;
    struct flake * flakes;
};



#define STEP__APPEND_COMBINATORIC          1
#define STEP__PACK                         2
#define STEP__OPTIMIZE                     3

struct step_data_append_combinatoric
{
    int n;
    int m;
};

struct step_data_pack
{
    pack_func * f;
};

struct step_data_optimize
{
    int nlayer;
};

union step_data
{
    struct step_data_append_combinatoric append_combinatoric;
    struct step_data_pack pack;
    struct step_data_optimize optimize;
};

struct step
{
    int type;
    struct step * next;
    union step_data data;
};



struct script
{
    struct mempool * restrict mempool;
    struct ofsm * restrict fsm;

    int status;
    struct step * step;
    struct step * last;
};

static struct script * create_script()
{
    struct mempool * restrict mempool = create_mempool(4000);
    if (mempool == NULL) {
        ERRHEADER;
        errmsg("create_mempool(4000) failed with NULL as return value.");
        return NULL;
    }

    struct script * restrict script = mempool_alloc(mempool, sizeof(struct script));
    if (script == NULL) {
        ERRHEADER;
        errmsg("mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct script));
        free_mempool(mempool);
        return NULL;
    }

    script->mempool = mempool;
    script->status = STATUS__NEW;
    script->step = NULL;
    script->last = NULL;
    return script;
}

static void free_script(struct script * restrict me)
{
    free_mempool(me->mempool);
}

static void save(struct script * restrict me)
{
}

static void do_append_combinatoric(struct script * restrict me, const struct step_data_append_combinatoric * args)
{
    verbose("START append combinatoric step, n = %d, m = %d.", args->n, args->m);
    verbose("DONE append combinatoric step.");
}

static void do_pack(struct script * restrict me, const struct step_data_pack * args)
{
    verbose("START pack step, f = %p.", args->f);
    verbose("DONE pack step.");
}

static void do_optimize(struct script * restrict me, const struct step_data_optimize * args)
{
    verbose("START optimize step, nlayer = %d.", args->nlayer);
    verbose("DONE optimize step.");
}

static void do_step(struct script * restrict me)
{
    if (me->step->type == STEP__APPEND_COMBINATORIC) {
        const struct step_data_append_combinatoric * args = &me->step->data.append_combinatoric;
        return do_append_combinatoric(me, args);
    }

    if (me->step->type == STEP__PACK) {
        const struct step_data_pack * args = &me->step->data.pack;
        return do_pack(me, args);
    }

    if (me->step->type == STEP__OPTIMIZE) {
        const struct step_data_optimize * args = &me->step->data.optimize;
        return do_optimize(me, args);
    }

    ERRHEADER;
    errmsg("Insupported step type: %d.", me->step->type);
    me->status = STATUS__FAILED;
}

static void run(struct script * restrict me)
{
    me->status = STATUS__EXECUTING;

    for (;;) {
        verbose("New step.");

        if (me->step == NULL) {
            me->status = STATUS__DONE;
            return;
        }

        if (opt_save_steps) {
            verbose("Start saving intermediate OFSM to file.");
            save(me);
            verbose("DONE saving.");
        }

        do_step(me);

        if (me->status == STATUS__INTERRUPTED) {
            verbose("Start saving current OFSM to file after interrupting.");
            save(me);
            verbose("DONE saving.");
            return;
        }

        if (me->status == STATUS__FAILED) {
            verbose("FAIL: current step turn status to error.");
            return;
        }

        me->step = me->step->next;
    }
}

static void print(struct script * restrict me)
{
}

int execute(int argc, char * argv[], build_script_func build)
{
    if (build == NULL) {
        ERRHEADER;
        errmsg("Invalid argument “f” = NULL (build script function). Not NULL value is expected.");
        return 1;
    }

    int errcode = parse_command_line(argc, argv);
    if (opt_help || errcode != 0) {
        usage();
        return errcode;
    }

    verbose("Start OFSM synthesis.");

    struct script * restrict script;
    script = create_script();
    if (script == NULL) {
        ERRHEADER;
        errmsg("create_script failed with NULL value. Terminate script execution.");
        return 1;
    }

    build(script);
    if (script->status == STATUS__FAILED) {
        errmsg("build(script) failed. Terminate script execution.");
    }
    verbose("DONE: building script.");

    run(script);
    verbose("DONE: running script.");

    if (script->status == STATUS__DONE) {
        print(script);
        verbose("DONE: output final OFSM.");
    }

    int exit_code = script->status == STATUS__DONE || script->status == STATUS__INTERRUPTED ? 0 : 1;
    free_script(script);
    verbose("Exit function execute.");
    return exit_code;
}

static struct step * append_step(struct script * restrict me)
{
    if (me->status == STATUS__FAILED) {
        /* One of previous step was failed. Ignore */
        return NULL;
    }

    struct step * restrict step = mempool_alloc(me->mempool, sizeof(struct step));
    if (step == NULL) {
        ERRHEADER;
        errmsg("mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct step));
        me->status = STATUS__FAILED;
        return NULL;
    }

    if (me->step == NULL) {
        me->step = step;
    } else {
        me->last->next = step;
    }

    step->next = NULL;
    me->last = step;
    return step;
}

static void add_step_append_combinatoric(struct script * restrict me, int n, int m)
{
    struct step * restrict step = me->last;
    step->type = STEP__APPEND_COMBINATORIC;
    struct step_data_append_combinatoric * restrict data = &step->data.append_combinatoric;
    data->n = n;
    data->m = m;
}

static void add_step_pack(struct script * restrict me, pack_func f)
{
    struct step * restrict step = me->last;
    step->type = STEP__PACK;
    struct step_data_pack * restrict data = data = &step->data.pack;
    data->f = f;
}

static void add_step_optimize(struct script * restrict me, int nlayer)
{
    struct step * restrict step = me->last;
    step->type = STEP__OPTIMIZE;
    struct step_data_optimize * restrict data = data = &step->data.optimize;
    data->nlayer = nlayer;
}

void script_append_combinatoric(void * restrict script, int n, int m)
{
    if (append_step(script) != NULL) {
        add_step_append_combinatoric(script, n, m);
    }
}

void script_pack(void * restrict script, pack_func f)
{
    if (append_step(script) != NULL) {
        add_step_pack(script, f);
    }
}

void script_optimize(void * restrict script, int nlayer)
{
    if (append_step(script) != NULL) {
        add_step_optimize(script, nlayer);
    }
}
