#include <ofsm.h>

#include <yoo-stdlib.h>
#include <yoo-combinatoric.h>

#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>






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
    void * ptr;
    uint64_t qinputs;
    uint64_t qoutputs;
    uint64_t qstates;
    state_t * data;
    input_t * paths;
};

static const struct flake zero_flake = { NULL, 0, 1, 0, NULL, NULL };

struct ofsm
{
    unsigned int qflakes;
    unsigned int max_flakes;
    struct flake * flakes;
};

struct ofsm * create_ofsm(struct mempool * restrict mempool, unsigned int max_flakes)
{
    if (max_flakes == 0) {
        max_flakes = 32;
    }

    struct ofsm * ofsm = mempool_alloc(mempool, sizeof(struct ofsm));
    if (ofsm == NULL) {
        ERRHEADER;
        errmsg("mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct ofsm));
        return NULL;
    }

    size_t sz = max_flakes * sizeof(struct flake);
    struct flake * flakes = mempool_alloc(mempool, sz);
    if (flakes == NULL) {
        ERRHEADER;
        errmsg("mempool_alloc(mempool, %lu) failed with NULL as return value.", sz);
        return NULL;
    }

    ofsm->qflakes = 1;
    ofsm->max_flakes = max_flakes;
    ofsm->flakes = flakes;
    ofsm->flakes[0] = zero_flake;
    return ofsm;
}

void free_ofsm(struct ofsm * restrict me)
{
    for (unsigned int i=0; i<me->qflakes; ++i) {
        void * ptr = me->flakes[i].ptr;
        if (ptr != NULL) {
            free(ptr);
        }
    }
}



struct choose_table
{
    unsigned int n;
    unsigned int m;
    uint64_t * data;
};

int init_choose_table(struct choose_table * restrict me, unsigned int n, unsigned int m)
{
    if (me->n >= n && me->m >= m) {
        return 0;
    }

    size_t sz = (n+1) * (m+1) * sizeof(uint64_t);
    void * restrict data = malloc(sz);
    if (data == NULL) {
        ERRHEADER;
        errmsg("malloc(%lu) fails with NULL value.", sz);
        return 1;
    }

    init_choose(data, n+1, m+1);

    if (me->data != NULL) {
        free(data);
    }

    me->n = n;
    me->m = m;
    me->data = data;
    return 0;
}

static void clear_choose_table(struct choose_table * restrict me)
{
    me->n = me->m = 0;
    if (me != NULL && me->data != NULL) {
        free(me->data);
    }
}

static uint64_t choose(const struct choose_table * me, unsigned int n, unsigned int m)
{
    if (n > me->n) return 0;
    if (m > me->m) return 0;
    size_t index = (me->m + 1) * n + m;
    return me->data[index];
}



#define STEP__APPEND_POWER                 1
#define STEP__APPEND_COMBINATORIC          2
#define STEP__PACK                         3
#define STEP__OPTIMIZE                     4

struct step_data_append_power
{
    unsigned int n;
    unsigned int m;
};

struct step_data_append_combinatoric
{
    unsigned int n;
    unsigned int m;
};

struct step_data_pack
{
    pack_func * f;
};

struct step_data_optimize
{
    int nflake;
};

union step_data
{
    struct step_data_append_power append_power;
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
    struct ofsm * restrict ofsm;

    int status;
    struct step * step;
    struct step * last;

    struct choose_table choose;
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

    struct ofsm * restrict ofsm = create_ofsm(mempool, 0);
    if (ofsm == NULL) {
        ERRHEADER;
        errmsg("create_ofsm(mempool, 0) failed with NULL value. Terminate script execution.");
        free_mempool(mempool);
        return NULL;
    }

    script->mempool = mempool;
    script->ofsm = ofsm;
    script->status = STATUS__NEW;
    script->step = NULL;
    script->last = NULL;
    memset(&script->choose, 0, sizeof(struct choose_table));
    return script;
}

static void free_script(struct script * restrict me)
{
    free_ofsm(me->ofsm);
    clear_choose_table(&me->choose);
    free_mempool(me->mempool);
}

static void save(struct script * restrict me)
{
}

static struct flake * create_flake(struct script * restrict me, uint64_t qinputs, uint64_t qoutputs, uint64_t qstates)
{
    struct ofsm * restrict ofsm = me->ofsm;

    unsigned int nflake = ofsm->qflakes;
    if (nflake >= ofsm->max_flakes) {
        ERRHEADER;
        errmsg("Overflow maximum flake count (%u), qflakes = %u.", ofsm->max_flakes, ofsm->qflakes);
        return NULL;
    }

    size_t sizes[3];
    sizes[0] = 0;
    sizes[1] = qinputs * qstates * sizeof(state_t);
    sizes[2] = qoutputs * nflake * sizeof(input_t);

    void * ptrs[3];
    verbose("  Try allocate data for flake %u: data_sz = %lu, paths_sz = %lu.", nflake, sizes[1], sizes[2]);
    multialloc(3, sizes, ptrs, 32);

    if (ptrs[0] == NULL) {
        ERRHEADER;
        errmsg("multialloc(3, {%lu, %lu, %lu}, ptrs, 32) failed for new flake.", sizes[0], sizes[1], sizes[2]);
        return NULL;
    }

    struct flake * restrict flake = ofsm->flakes + nflake;
    verbose("  Allocation OK, ptr = %p.", ptrs[0]);

    flake->qinputs = qinputs;
    flake->qoutputs = qoutputs;
    flake->qstates = qstates;
    flake->ptr = ptrs[0];
    flake->data = ptrs[1];
    flake->paths = ptrs[2];
    verbose("  New flake %u: qinputs = %lu, qoutputs = %lu, qstates = %lu.", nflake, qinputs, qoutputs, qstates);

    ++ofsm->qflakes;
    return flake;
}

static int calc_paths(const struct flake * flake, unsigned int nflake)
{
    const uint64_t qinputs = flake->qinputs;
    const uint64_t qstates = flake->qstates;
    const uint64_t qoutputs = flake->qoutputs;

    {
        // May be not required, this is just optimization.
        // To catch errors we fill data

        const input_t invalid_value = ~(input_t)0;
        input_t * restrict ptr = flake->paths;
        const input_t * end = ptr + nflake * qoutputs;
        for (; ptr != end; ++ptr) {
            *ptr = invalid_value;
        }
    }

    if (nflake < 1) {
        ERRHEADER;
        errmsg("Assertion failed: invalid nflake value (%u). It should be positive (1, 2, 3...).", nflake);
        return 1;
    }

    if (nflake == 1 && qstates != 1) {
        ERRHEADER;
        errmsg("Assertion failed: for nflake = 1 qstates (%lu) should be 1.", qstates);
        return 1;
    }

    const struct flake * prev = flake - 1;
    const state_t * data_ptr = flake->data;

    if (nflake > 1) {

        // Common version
        for (uint64_t state=0; state<qstates; ++state) {
            const input_t * base = prev->paths + state * (nflake-1);
            for (input_t input=0; input < qinputs; ++input) {
                state_t output = *data_ptr++;
                if (output == INVALID_STATE) continue;
                input_t * restrict ptr = flake->paths + output * nflake;
                memcpy(ptr, base, (nflake-1) * sizeof(input_t));
                ptr[nflake-1] = input;
            }
        }

    } else {

        // Optimized version, only one state, no copying previous paths
        for (input_t input=0; input < qinputs; ++input) {
            state_t output = *data_ptr++;
            input_t * restrict ptr = flake->paths + output;
            *ptr = input;
        }
    }

    return 0;
}

static void do_append_power_flake(struct script * restrict me, unsigned int n)
{
    verbose("  Append flake with %u inputs.", n);

    struct ofsm * restrict ofsm = me->ofsm;
    unsigned int nflake = ofsm->qflakes;
    const struct flake * prev = ofsm->flakes + nflake - 1;

    uint64_t qinputs = n;
    uint64_t qstates = prev->qoutputs;
    uint64_t qoutputs = qstates * n;

    const struct flake * flake = create_flake(me, qinputs, qoutputs, qstates);
    if (flake == NULL) {
        ERRHEADER;
        errmsg("  create_flake(me, %lu, %lu, %lu) faled with NULL as return value.", qinputs, qoutputs, qstates);
        me->status = STATUS__FAILED;
        return;
    }

    state_t * restrict data = flake->data;
    state_t output = 0;

    for (uint64_t state=0; state<qstates; ++state)
    for (uint64_t input=0; input < n; ++input) {
        *data++ = output++;
    }

    int errcode = calc_paths(flake, nflake);
    if (errcode != 0) {
        ERRHEADER;
        errmsg("  calc_paths(flake, %u) failed with %d as an error code.", nflake, errcode);
        me->status = STATUS__FAILED;
        return;
    }
}

static void do_append_power(struct script * restrict me, const struct step_data_append_power * args)
{
    verbose("START append power step, n = %u, m = %u.", args->n, args->n);

    for (unsigned int i=0; i<args->m; ++i) {
        if (me->status != STATUS__FAILED) {
            do_append_power_flake(me, args->n);
        }
    }

    verbose("DONE append power step.");
}

static state_t calc_combinatoric_index(const struct choose_table * ct, const input_t * inputs, unsigned int n, unsigned int m)
{

    if (n > 64) {
        ERRHEADER;
        errmsg("Sorting of inputs more then 64 is not supported yet.");
        return INVALID_STATE;
    }

    uint64_t mask = 0;
    for (unsigned int i=0; i<m; ++i) {
        uint64_t new_one = 1ull << inputs[i];
        if (new_one & mask) return INVALID_STATE; // repetition
        mask |= new_one;
    }

    input_t sorted[m];
    for (unsigned int i = 0; i<m; ++i) {
        sorted[i] = extract_rbit64(&mask);
    }

    /*
    unsigned int i = m;
    while (i-->0) {
        sorted[i] = extract_rbit64(&mask);
    }
    */

    state_t result = 0;
    for (unsigned int k = 0; k < m; ++k) {
        result += choose(ct, sorted[k], k+1);
    }

    return result;
}

static void do_append_combinatoric(struct script * restrict me, const struct step_data_append_combinatoric * args)
{
    verbose("START append combinatoric step, n = %d, m = %d.", args->n, args->m);

    struct choose_table * restrict ct = &me->choose;
    init_choose_table(ct, args->n, args->m);

    struct ofsm * restrict ofsm = me->ofsm;
    const struct flake * prev =  ofsm->flakes + ofsm->qflakes - 1;

    uint64_t nn = args->n;
    uint64_t dd = 1;
    uint64_t N = prev->qoutputs;

    for (int i=0; i<args->m; ++i) {

        uint64_t qinputs = args->n;
        uint64_t qstates = prev->qoutputs;
        uint64_t qoutputs = qstates * nn / dd;

        unsigned int nflake = ofsm->qflakes;
        const struct flake * flake = create_flake(me, qinputs, qoutputs, qstates);

        if (flake == NULL) {
            ERRHEADER;
            errmsg("  create_flake(me, %lu, %lu, %lu) faled with NULL as return value.", qinputs, qoutputs, qstates);
            me->status = STATUS__FAILED;
            return;
        }

        state_t * restrict data = flake->data;
        uint64_t unique_state_count = qstates / N;
        uint64_t unique_output_count = qoutputs / N;

        for (uint64_t state = 0; state < unique_state_count; ++state) {
            input_t c[i+1];
            if (i > 0) {
                const input_t * ptr = prev->paths + (nflake-1) * (state+1) - i;
                memcpy(c, ptr, i * sizeof(input_t));
            }

            for (uint64_t input = 0; input < qinputs; ++input) {
                c[i] = input;
                *data++ = calc_combinatoric_index(ct, c, args->n, i+1);
            }
        }

        int64_t offset = flake->data - data;
        const state_t * end = flake->data + qinputs * qstates;
        for (; data != end; ++data) {
            state_t state = data[offset];
            if (state != INVALID_STATE) {
                state += unique_output_count;
            }
            *data = state;
        }

        int errcode = calc_paths(flake, nflake);
        if (errcode != 0) {
            ERRHEADER;
            errmsg("  calc_paths(flake, %u) failed with %d as an error code.", nflake, errcode);
            me->status = STATUS__FAILED;
            return;
        }

        prev = flake;

        --nn;
        ++dd;
    }

    verbose("DONE append combinatoric step.");
}

struct ofsm_pack_decode {
    state_t output;
    pack_value_t value;
};

static int cmp_ofsm_pack_decode(const void * arg_a, const void * arg_b)
{
    const struct ofsm_pack_decode * a = arg_a;
    const struct ofsm_pack_decode * b = arg_b;
    if (a->value < b->value) return -1;
    if (a->value > b->value) return +1;
    if (a->output < b->output) return -1;
    if (a->output > b->output) return +1;
    return 0;
}

static void do_pack(struct script * restrict me, const struct step_data_pack * args)
{
    verbose("START pack step, f = %p.", args->f);



    struct ofsm * restrict ofsm = me->ofsm;
    if (ofsm->qflakes <= 1) {
        ERRHEADER;
        errmsg("Try to pack empty OFSM.");
        me->status = STATUS__FAILED;
        return;
    }



    unsigned int nflake = ofsm->qflakes - 1;
    const struct flake oldman =  ofsm->flakes[nflake];
    uint64_t old_output_count = oldman.qoutputs;



    size_t sizes[3];
    sizes[0] = 0;
    sizes[1] = old_output_count * sizeof(struct ofsm_pack_decode);
    sizes[2] = old_output_count * sizeof(state_t);
    sizes[3] = old_output_count * sizeof(state_t);

    void * ptrs[4];
    multialloc(4, sizes, ptrs, 32);
    void * ptr = ptrs[0];

    if (ptr == NULL) {
        ERRHEADER;
        errmsg("  multialloc(3, {%lu, %lu, %lu}, ptrs, 32) failed for temporary packing data.", sizes[0], sizes[1], sizes[2]);
        me->status = STATUS__FAILED;
        return;
    }

    struct ofsm_pack_decode * decode_table = ptrs[1];
    state_t * translate = ptrs[2];
    state_t * backref = ptrs[3];



    { verbose("  --> calculate pack values.");

        struct ofsm_pack_decode * restrict curr = decode_table;
        const input_t * path = oldman.paths;
        for (size_t output = 0; output < old_output_count; ++output) {
            curr->output = output;
            curr->value = args->f(nflake, path);
            path += nflake;
            ++curr;
        }

    } verbose("  <<< calculate pack values.");



    { verbose("  --> sort new states.");
        qsort(decode_table, old_output_count, sizeof(struct ofsm_pack_decode), &cmp_ofsm_pack_decode);
    } verbose("  <<< sort new states.");




    state_t new_output_count = 0;

    { verbose("  --> calc output_translate table.");

        const struct ofsm_pack_decode * left = decode_table;
        const struct ofsm_pack_decode * end = decode_table + old_output_count;

        while (left != end) {

            const struct ofsm_pack_decode * right = left + 1;
            while (right != end && right->value == left->value) {
                ++right;
            }

            state_t new_output;
            if (left->value != INVALID_PACK_VALUE) {
                new_output = new_output_count++;
                backref[new_output] = left->output;
            } else {
                new_output = INVALID_STATE;
            }

            for (; left != right; ++left) {
                translate[left->output] = new_output;
            }
        }

    } verbose("  <<< calc output_translate table.");



    --ofsm->qflakes;
    struct flake * restrict infant = create_flake(me, oldman.qinputs, new_output_count, oldman.qstates);
    if (infant == NULL) {
        ERRHEADER;
        errmsg("create_flake(me, %lu, %u, %lu) faled with NULL as return value in pack step.", oldman.qinputs, new_output_count, oldman.qstates);
        me->status = STATUS__FAILED;
        free(ptr);
        return;
    }



    { verbose("  --> update data.");

        const state_t * old = oldman.data;
        const state_t * end = old + oldman.qstates * oldman.qinputs;
        state_t * restrict new = infant->data;

        for (; old != end; ++old) {
            if (*old != INVALID_STATE) {
                *new++ = translate[*old];
            } else {
                *new++ = INVALID_STATE;
            }
        }

    } verbose("  <<< update data.");



    { verbose("  --> update paths.");

        size_t sz = sizeof(input_t) * nflake;
        input_t * restrict new_path = infant->paths;
        for (state_t new_output = 0; new_output < new_output_count; ++new_output) {
            state_t old_output = backref[new_output];
            memcpy(new_path,  oldman.paths + old_output * nflake, sz);
            new_path += nflake;
        }

    } verbose("  <<< update paths.");



    free(oldman.ptr);
    free(ptr);
    verbose("DONE pack step.");
}



static void do_optimize(struct script * restrict me, const struct step_data_optimize * args)
{
    struct ofsm * restrict ofsm = me->ofsm;
    if (ofsm->qflakes <= 1) {
        ERRHEADER;
        errmsg("Try to optmimize empty OFSM.");
        me->status = STATUS__FAILED;
        return;
    }

    unsigned int nflake = args->nflake;
    if (nflake <= 0 || nflake >= ofsm->qflakes) {
        ERRHEADER;
        errmsg("Invalid flake number (%u), expected value might be in range 1 - %u.", nflake, ofsm->qflakes - 1);
        me->status = STATUS__FAILED;
        return;
    }

    verbose("START optimize step, nlayer  %d.", args->nflake);

    verbose("DONE optimize step.");
}

static void do_step(struct script * restrict me)
{
    const void * args = &me->step->data;

    switch (me->step->type) {
        case STEP__APPEND_POWER:           return do_append_power(me, args);
        case STEP__APPEND_COMBINATORIC:    return do_append_combinatoric(me, args);
        case STEP__PACK:                   return do_pack(me, args);
        case STEP__OPTIMIZE:               return do_optimize(me, args);
        default: break;
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

int do_ofsm_execute(const struct ofsm * me, unsigned int n, const int * inputs)
{
    if (n >= me->qflakes) {
        ERRHEADER;
        errmsg("Maximum input size is equal to qflakes = %u, but %u as passed as argument n.", me->qflakes, n);
        return -1;
    }

    for (int i=0; i<n; ++i) {
        const struct flake * flake = me->flakes + i + 1;
        if (inputs[i] >= flake->qinputs) {
            ERRHEADER;
            errmsg("Invalid input, the inputs[%d] = %u, it increases flake qinputs (%lu).", i, inputs[i], flake->qinputs);
            return -1;
        }
    }

    state_t state = 0;
    const struct flake * flake = me->flakes + 1;

    for (int i=0; i<n; ++i) {
        input_t input = inputs[i];

        state = flake->data[state * flake->qinputs + input];
        if (state == INVALID_STATE) return -1;
        if (state >= flake->qoutputs) {
            ERRHEADER;
            errmsg("Invalid OFSM, new state = %u more than qoutputs = %lu.", state, flake->qoutputs);
            return -1;
        }

        ++flake;
    }

    return state;
}

static void print(struct script * restrict me)
{
}

int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check)
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
        if (check != NULL) {
            errcode = check(script->ofsm);
            if (errcode != 0) {
                errmsg("Check failed with code %d.", errcode);
                script->status = STATUS__FAILED;
            }
        }

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

static void add_step_append_power(struct script * restrict me, unsigned int n, unsigned int m)
{
    struct step * restrict step = me->last;
    step->type = STEP__APPEND_POWER;
    struct step_data_append_power * restrict data = &step->data.append_power;
    data->n = n;
    data->m = m;
}

static void add_step_append_combinatoric(struct script * restrict me, unsigned int n, unsigned int m)
{
    struct step * restrict step = me->last;
    step->type = STEP__APPEND_COMBINATORIC;
    struct step_data_append_combinatoric * restrict data = &step->data.append_combinatoric;
    data->n = n;
    data->m = m;
}

static void add_step_append_pack(struct script * restrict me, pack_func f)
{
    struct step * restrict step = me->last;
    step->type = STEP__PACK;
    struct step_data_pack * restrict data = data = &step->data.pack;
    data->f = f;
}

static void add_step_optimize(struct script * restrict me, unsigned int nflake)
{
    struct step * restrict step = me->last;
    step->type = STEP__OPTIMIZE;
    struct step_data_optimize * restrict data = data = &step->data.optimize;
    data->nflake = nflake;
}

void script_append_power(void * restrict script, unsigned int n, unsigned int m)
{
    if (append_step(script) != NULL) {
        add_step_append_power(script, n, m);
    }
}

void script_append_combinatoric(void * restrict script, unsigned int n, unsigned int m)
{
    if (append_step(script) != NULL) {
        add_step_append_combinatoric(script, n, m);
    }
}

void script_append_pack(void * restrict script, pack_func f)
{
    if (append_step(script) != NULL) {
        add_step_append_pack(script, f);
    }
}

void script_optimize(void * restrict script, unsigned int nflake)
{
    if (append_step(script) != NULL) {
        add_step_optimize(script, nflake);
    }
}

void script_fail(void * restrict script)
{
    struct script * restrict me = script;
    me->status = STATUS__FAILED;
}

int ofsm_execute(const void * ofsm, unsigned int n, const int * inputs)
{
    return do_ofsm_execute(ofsm, n, inputs);
}
