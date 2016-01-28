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
    int status;
    struct step * step;
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
    return script;
}

static void free_script(struct script * restrict me)
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
        if (me->step == NULL) {
            me->status = STATUS__DONE;
            return;
        }

        if (opt_save_steps) {
            save(me);
        }

        do_step(me);

        if (me->status == STATUS__INTERRUPTED) {
            save(me);
            return;
        }

        if (me->status == STATUS__FAILED) {
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
    if (script->status == STATUS__FAILED) {
        errmsg("build(script) failed. Terminate script execution.");
    }

    run(script);

    if (script->status == STATUS__DONE) {
        print(script);
    }

    free_script(script);
    return 0;
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

    step->next = me->step;
    return me->step = step;
}

static void add_step_append_combinatoric(struct script * restrict me, int n, int m)
{
    struct step * restrict step = me->step;
    step->type = STEP__APPEND_COMBINATORIC;
    struct step_data_append_combinatoric * restrict data = &step->data.append_combinatoric;
    data->n = n;
    data->m = m;
}

static void add_step_pack(struct script * restrict me, pack_func f)
{
    struct step * restrict step = me->step;
    step->type = STEP__PACK;
    struct step_data_pack * restrict data = data = &step->data.pack;
    data->f = f;
}

static void add_step_optimize(struct script * restrict me, int nlayer)
{
    struct step * restrict step = me->step;
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
