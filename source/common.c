#include <fsm.h>

#include <yoo-stdlib.h>

#include <stdarg.h>
#include <stdio.h>

#define STATUS__NEW           1
#define STATUS__EXECUTING     2
#define STATUS__FAILED        3
#define STATUS__INTERRUPTED   4
#define STATUS__DONE          5



#define ERRHEADER errmsg("Error in function “%s”, file %s:%d.", __FUNCTION__, __FILE__, __LINE__)

static void verrmsg(const char * fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static void errmsg(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void errmsg(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verrmsg(fmt, args);
    va_end(args);
}



int opt_save_steps = 0;

static int parse_command_line(int argc, char * argv[])
{
    return 0;
}



struct script
{
    struct mempool * restrict mempool;
    int status;
    int nstep;
    int qstep;
};

struct script * create_script()
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
    script->nstep = script->qstep = 0;
    return script;
}

void free_script(struct script * restrict me)
{
    free_mempool(me->mempool);
}

static void save(struct script * restrict me)
{
}

static void do_step(struct script * restrict me)
{
}

static void run(struct script * restrict me)
{
    me->status = STATUS__EXECUTING;

    for (;;) {
        if (me->nstep >= me->qstep) {
            me->status = STATUS__DONE;
            return;
        }

        do_step(me);

        if (me->status == STATUS__INTERRUPTED) {
            save(me);
            return;
        }

        if (me->status == STATUS__FAILED) {
            return;
        }

        ++me->nstep;

        if (opt_save_steps) {
            save(me);
        }
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
    if (errcode != 0) {
        return errcode;
    }

    struct script * restrict script;
    script = create_script();
    if (script == NULL) {
        ERRHEADER;
        errmsg("create_script failed with NULL value. Terminate script execution.");
        return 1;
    }

    build(script);

    run(script);

    if (script->status == STATUS__DONE) {
        print(script);
    }

    free_script(script);
    return 0;
}

void script_append_combinatoric(void * restrict script, int n, int m)
{
}

void script_pack(void * restrict script, pack_func pack)
{
}

void script_optimize(void * restrict script, int layer)
{
}
