#include "yoo-ofsm.h"

#include <yoo-stdlib.h>
#include <yoo-combinatoric.h>

#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>






#define STATUS__NEW           1
#define STATUS__EXECUTING     2
#define STATUS__FAILED        3
#define STATUS__INTERRUPTED   4
#define STATUS__DONE          5



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
                    msg(stderr, "Invalid option.");
                    return 1;
                default:
                    msg(stderr, "getopt_long returns unexpected char \\x%02X.", c);
                    return 1;
            }
        }
    }

    if (opt_verbose) {
        verbose_stream = stderr;
    }

    if (optind < argc) {
        msg(stderr, "Function “execute” do not expected any extra command line arguments except options.");
        verbose(stderr, "optind = %d, argc = %d.", optind, argc);
        return 1;
    }


    return 0;
}






struct flake
{
    input_t qinputs;
    state_t qstates;
    state_t qoutputs;
    state_t * jumps[2];
    input_t * paths[2];
};

static const struct flake zero_flake = { 0, 0, 1, { NULL, NULL }, { NULL, NULL } };

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
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct ofsm));
        return NULL;
    }

    size_t sz = max_flakes * sizeof(struct flake);
    struct flake * flakes = mempool_alloc(mempool, sz);
    if (flakes == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sz);
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
    void * ptr;
    for (unsigned int i=0; i<me->qflakes; ++i) {

        ptr = me->flakes[i].jumps[0];
        if (ptr != NULL) {
            free(ptr);
        }

        ptr = me->flakes[i].paths[0];
        if (ptr) {
            free(ptr);
        }
    }
}



#define STEP__GROW_POW                     1
#define STEP__GROW_COMB                    2
#define STEP__PACK                         3
#define STEP__OPTIMIZE                     4

struct step_data_pow
{
    input_t qinputs;
    unsigned int m;
};

struct step_data_comb
{
    input_t qinputs;
    unsigned int m;
};

struct step_data_pack
{
    pack_func * f;
    unsigned int flags;
};

struct step_data_optimize
{
    int nflake;
    hash_func * f;
};

union step_data
{
    struct step_data_pow pow;
    struct step_data_comb comb;
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
    FILE * cfile;
};

static struct script * create_script()
{
    struct mempool * restrict mempool = create_mempool(4000);
    if (mempool == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "create_mempool(4000) failed with NULL as return value.");
        return NULL;
    }

    struct script * restrict script = mempool_alloc(mempool, sizeof(struct script));
    if (script == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct script));
        free_mempool(mempool);
        return NULL;
    }

    struct ofsm * restrict ofsm = create_ofsm(mempool, 0);
    if (ofsm == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "create_ofsm(mempool, 0) failed with NULL value. Terminate script execution.");
        free_mempool(mempool);
        return NULL;
    }

    script->mempool = mempool;
    script->ofsm = ofsm;
    script->status = STATUS__NEW;
    script->step = NULL;
    script->last = NULL;
    memset(&script->choose, 0, sizeof(struct choose_table));
    script->cfile = stdout;
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

static struct flake * create_flake(struct script * restrict me, input_t qinputs, uint64_t qoutputs, state_t qstates)
{
    struct ofsm * restrict ofsm = me->ofsm;

    unsigned int nflake = ofsm->qflakes;
    if (nflake >= ofsm->max_flakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Overflow maximum flake count (%u), qflakes = %u.", ofsm->max_flakes, ofsm->qflakes);
        return NULL;
    }

    void * jump_ptrs[2];
    void * path_ptrs[2];
    size_t jump_sizes[2] = { 0, qinputs * qstates * sizeof(state_t) };
    size_t path_sizes[2] = { 0, qoutputs * nflake * sizeof(input_t) };
    size_t total_sz = jump_sizes[1] + path_sizes[1];

    verbose(stderr, "  Try allocate data for jump table for flake %u: size = %lu.", nflake, jump_sizes[1]);
    multialloc(2, jump_sizes, jump_ptrs, 32);

    if (jump_ptrs[0] == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "multialloc(2, {%lu, %lu}, ptrs, 32) failed for new flake.", jump_sizes[0], jump_sizes[1]);
        return NULL;
    }

    verbose(stderr, "  Try allocate data for path table for flake %u: size = %lu.", nflake, path_sizes[1]);
    multialloc(2, path_sizes, path_ptrs, 32);

    if (path_ptrs[0] == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "multialloc(2, {%lu, %lu}, ptrs, 32) failed for new flake.", path_sizes[0], path_sizes[1]);
        free(jump_ptrs[0]);
        return NULL;
    }

    verbose(stderr, "  Allocation OK, jump_ptr = %p, path_ptr = %p.", jump_ptrs[0], path_ptrs[1]);
    struct flake * restrict flake = ofsm->flakes + nflake;

    flake->qinputs = qinputs;
    flake->qoutputs = qoutputs;
    flake->qstates = qstates;
    flake->jumps[0] = jump_ptrs[0];
    flake->jumps[1] = jump_ptrs[1];
    flake->paths[0] = path_ptrs[0];
    flake->paths[1] = path_ptrs[1];

    verbose(stderr, "  New flake %u: qinputs = %u, qoutputs = %lu, qstates = %u, total_sz = %lu.", nflake, qinputs, qoutputs, qstates, total_sz);

    ++ofsm->qflakes;
    return flake;
}

static int calc_paths(const struct flake * flake, unsigned int nflake)
{
    const input_t qinputs = flake->qinputs;
    const state_t qstates = flake->qstates;
    const uint64_t qoutputs = flake->qoutputs;

    {
        // May be not required, this is just optimization.
        // To catch errors we fill data

        input_t * restrict ptr = flake->paths[1];
        const input_t * end = ptr + nflake * qoutputs;
        for (; ptr != end; ++ptr) {
            *ptr = INVALID_INPUT;
        }
    }

    if (nflake < 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Assertion failed: invalid nflake value (%u). It should be positive (1, 2, 3...).", nflake);
        return 1;
    }

    if (nflake == 1 && qstates != 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Assertion failed: for nflake = 1 qstates (%u) should be 1.", qstates);
        return 1;
    }

    const struct flake * prev = flake - 1;
    const state_t * data_ptr = flake->jumps[1];

    if (nflake > 1) {

        // Common version
        for (state_t state=0; state<qstates; ++state) {
            const input_t * base = prev->paths[1] + state * (nflake-1);
            for (input_t input=0; input < qinputs; ++input) {
                state_t output = *data_ptr++;
                if (output == INVALID_STATE) continue;
                input_t * restrict ptr = flake->paths[1] + output * nflake;
                memcpy(ptr, base, (nflake-1) * sizeof(input_t));
                ptr[nflake-1] = input;
            }
        }

    } else {

        // Optimized version, only one state, no copying previous paths
        for (input_t input=0; input < qinputs; ++input) {
            state_t output = *data_ptr++;
            input_t * restrict ptr = flake->paths[1] + output;
            *ptr = input;
        }
    }

    return 0;
}

static void do_pow_flake(struct script * restrict me, input_t qinputs)
{
    verbose(stderr, "  Append flake with %u inputs.", qinputs);

    struct ofsm * restrict ofsm = me->ofsm;
    unsigned int nflake = ofsm->qflakes;
    const struct flake * prev = ofsm->flakes + nflake - 1;

    state_t qstates = prev->qoutputs;
    uint64_t qoutputs = qstates * qinputs;

    const struct flake * flake = create_flake(me, qinputs, qoutputs, qstates);
    if (flake == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "  create_flake(me, %u, %lu, %u) faled with NULL as return value.", qinputs, qoutputs, qstates);
        me->status = STATUS__FAILED;
        return;
    }

    state_t * restrict jumps = flake->jumps[1];
    state_t output = 0;

    for (uint64_t state=0; state<qstates; ++state)
    for (uint64_t input=0; input<qinputs; ++input) {
        *jumps++ = output++;
    }

    int errcode = calc_paths(flake, nflake);
    if (errcode != 0) {
        ERRLOCATION(stderr);
        msg(stderr, "  calc_paths(flake, %u) failed with %d as an error code.", nflake, errcode);
        me->status = STATUS__FAILED;
        return;
    }
}

static void do_pow(struct script * restrict me, const struct step_data_pow * args)
{
    verbose(stderr, "START append power step, qinputs = %u, m = %u.", args->qinputs, args->m);

    for (unsigned int i=0; i<args->m; ++i) {
        if (me->status != STATUS__FAILED) {
            do_pow_flake(me, args->qinputs);
        }
    }

    verbose(stderr, "DONE append power step.");
}

static state_t calc_comb_index(const struct choose_table * ct, const input_t * inputs, unsigned int qinputs, unsigned int m)
{

    if (qinputs > 64) {
        ERRLOCATION(stderr);
        msg(stderr, "Sorting of inputs more then 64 is not supported yet.");
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

    state_t result = 0;
    for (unsigned int k = 0; k < m; ++k) {
        result += choose(ct, sorted[k], k+1);
    }

    return result;
}

static void do_comb(struct script * restrict me, const struct step_data_comb * args)
{
    input_t qinputs = args->qinputs;

    verbose(stderr, "START append combinatoric step, qinputs = %u, m = %u.", args->qinputs, args->m);

    struct choose_table * restrict ct = &me->choose;
    init_choose_table(ct, args->qinputs, args->m, stderr);

    struct ofsm * restrict ofsm = me->ofsm;
    const struct flake * prev =  ofsm->flakes + ofsm->qflakes - 1;

    uint64_t nn = args->qinputs;
    uint64_t dd = 1;
    uint64_t N = prev->qoutputs;

    for (int i=0; i<args->m; ++i) {

        if (prev->qoutputs >= INVALID_STATE) {
            ERRLOCATION(stderr);
            msg(stderr, "state_t overflow: try to assign %u to qstates.", prev->qoutputs);
            me->status = STATUS__FAILED;
            return;
        }

        state_t qstates = prev->qoutputs;
        uint64_t qoutputs = qstates * nn / dd;

        unsigned int nflake = ofsm->qflakes;
        const struct flake * flake = create_flake(me, qinputs, qoutputs, qstates);

        if (flake == NULL) {
            ERRLOCATION(stderr);
            msg(stderr, "  create_flake(me, %u, %lu, %u) faled with NULL as return value.", qinputs, qoutputs, qstates);
            me->status = STATUS__FAILED;
            return;
        }

        state_t * restrict jumps = flake->jumps[1];
        state_t unique_state_count = qstates / N;
        uint64_t unique_output_count = qoutputs / N;

        for (state_t state = 0; state < unique_state_count; ++state) {
            input_t c[i+1];
            if (i > 0) {
                const input_t * ptr = prev->paths[1] + (nflake-1) * (state+1) - i;
                memcpy(c, ptr, i * sizeof(input_t));
            }

            for (uint64_t input = 0; input < qinputs; ++input) {
                c[i] = input;
                *jumps++ = calc_comb_index(ct, c, args->qinputs, i+1);
            }
        }

        int64_t offset = flake->jumps[1] - jumps;
        const state_t * end = flake->jumps[1] + qinputs * qstates;
        for (; jumps != end; ++jumps) {
            state_t state = jumps[offset];
            if (state != INVALID_STATE) {
                state += unique_output_count;
            }
            *jumps = state;
        }

        int errcode = calc_paths(flake, nflake);
        if (errcode != 0) {
            ERRLOCATION(stderr);
            msg(stderr, "  calc_paths(flake, %u) failed with %d as an error code.", nflake, errcode);
            me->status = STATUS__FAILED;
            return;
        }

        prev = flake;

        --nn;
        ++dd;
    }

    verbose(stderr, "DONE append combinatoric step.");
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
    int skip_renumering = (args->flags & PACK_FLAG__SKIP_RENUMERING) != 0;

    verbose(stderr, "START pack step, skip_renumering = %d, f = %p.", skip_renumering, args->f);



    struct ofsm * restrict ofsm = me->ofsm;
    if (ofsm->qflakes <= 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Try to pack empty OFSM.");
        me->status = STATUS__FAILED;
        return;
    }



    unsigned int nflake = ofsm->qflakes - 1;
    const struct flake oldman =  ofsm->flakes[nflake];
    uint64_t old_qoutputs = oldman.qoutputs;



    size_t sizes[3];
    sizes[0] = 0;
    sizes[1] = (1 + old_qoutputs) * sizeof(struct ofsm_pack_decode);
    sizes[2] = old_qoutputs * sizeof(state_t);

    void * ptrs[3];
    multialloc(3, sizes, ptrs, 32);
    void * ptr = ptrs[0];

    if (ptr == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "  multialloc(4, {%lu, %lu, %lu}, ptrs, 32) failed for temporary packing data.", sizes[0], sizes[1], sizes[2]);
        me->status = STATUS__FAILED;
        return;
    }

    struct ofsm_pack_decode * decode_table = ptrs[1];
    state_t * translate = ptrs[2];



    pack_value_t max_value = 0;

    { verbose(stderr, "  --> calculate pack values.");

        struct ofsm_pack_decode * restrict curr = decode_table;
        const input_t * path = oldman.paths[1];
        for (size_t output = 0; output < old_qoutputs; ++output) {
            curr->output = output;
            curr->value = args->f(nflake, path);
            if (curr->value > max_value) max_value = curr->value;
            path += nflake;
            ++curr;
        }

        curr->output = INVALID_STATE;
        curr->value = INVALID_PACK_VALUE;

    } verbose(stderr, "  <<< calculate pack values, max value is %lu.", max_value);



    { verbose(stderr, "  --> sort new states.");
        qsort(decode_table, old_qoutputs, sizeof(struct ofsm_pack_decode), &cmp_ofsm_pack_decode);
    } verbose(stderr, "  <<< sort new states.");




    state_t new_qoutputs = 0;

    if (skip_renumering) {
        verbose(stderr, "  --> calc output_translate table without renumering.");

        new_qoutputs = max_value + 1;

        const struct ofsm_pack_decode * left = decode_table;
        const struct ofsm_pack_decode * end = decode_table + old_qoutputs;

        while (left != end) {

            const struct ofsm_pack_decode * right = left + 1;
            while (right != end && right->value == left->value) {
                ++right;
            }

            state_t new_output = left->value != INVALID_PACK_VALUE ? left->value : INVALID_STATE;
            for (; left != right; ++left) {
                translate[left->output] = new_output;
            }
        }

        verbose(stderr, "  <<< calc output_translate table without renumering.");

    } else {
        verbose(stderr, "  --> calc output_translate table with renumering.");

        const struct ofsm_pack_decode * left = decode_table;
        const struct ofsm_pack_decode * end = decode_table + old_qoutputs;

        while (left != end) {

            const struct ofsm_pack_decode * right = left + 1;
            while (right != end && right->value == left->value) {
                ++right;
            }

            state_t new_output;
            if (left->value != INVALID_PACK_VALUE) {
                new_output = new_qoutputs++;
            } else {
                new_output = INVALID_STATE;
            }

            for (; left != right; ++left) {
                translate[left->output] = new_output;
            }
        }

        verbose(stderr, "  <<< calc output_translate table with renumering.");
    }



    --ofsm->qflakes;
    struct flake * restrict infant = create_flake(me, oldman.qinputs, new_qoutputs, oldman.qstates);
    if (infant == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "create_flake(me, %u, %u, %u) faled with NULL as return value in pack step.", oldman.qinputs, new_qoutputs, oldman.qstates);
        me->status = STATUS__FAILED;
        free(ptr);
        return;
    }



    { verbose(stderr, "  --> update data.");

        const state_t * old = oldman.jumps[1];
        const state_t * end = old + oldman.qstates * oldman.qinputs;
        state_t * restrict new = infant->jumps[1];

        for (; old != end; ++old) {
            if (*old != INVALID_STATE) {
                *new++ = translate[*old];
            } else {
                *new++ = INVALID_STATE;
            }
        }

    } verbose(stderr, "  <<< update data.");



    { verbose(stderr, "  --> update paths.");

        input_t * restrict new_path = infant->paths[1];
        const input_t * old_paths = oldman.paths[1];
        const struct ofsm_pack_decode * decode = decode_table;
        size_t sz = sizeof(input_t) * nflake;
        for (state_t output = 0; output < new_qoutputs; ++output) {
            pack_value_t value = decode->value;
            state_t old_output = decode->output;
            state_t new_output = translate[old_output];
            if (output == new_output) {
                memcpy(new_path, old_paths + old_output * nflake, sz);
                new_path += nflake;
                do ++decode; while (decode->value == value);
            } else {
                for (unsigned int i=0; i < nflake; ++i) {
                    *new_path++ = INVALID_INPUT;
                }
            }
        }

    } verbose(stderr, "  <<< update paths.");



    free(oldman.jumps[0]);
    free(oldman.paths[0]);
    free(ptr);
    verbose(stderr, "DONE pack step, new qoutputs = %u.", new_qoutputs);
}



struct state_info
{
    state_t old;
    state_t new;
    uint64_t hash;
    state_t index;
};

static int cmp_state_info(const void * arg_a, const void * arg_b)
{
    const struct state_info * a = arg_a;
    const struct state_info * b = arg_b;
    if (a->hash < b->hash) return -1;
    if (a->hash > b->hash) return +1;
    if (a->old < b->old) return -1;
    if (a->old > b->old) return +1;
    return 0;
}

int merge(unsigned int qinputs, state_t * restrict a, state_t * restrict b)
{
    for (unsigned int i = 0; i < qinputs; ++i) {
        if (a[i] == INVALID_STATE) continue;
        if (b[i] == INVALID_STATE) continue;
        if (a[i] != b[i]) return 0;
    }

    for (unsigned int i = 0; i < qinputs; ++i) {
        if (a[i] == INVALID_STATE) {
            a[i] = b[i];
        }
    }

    return 1;
}

static uint64_t get_first_jump(unsigned int n, const state_t * jumps)
{
    return *jumps != INVALID_STATE ? *jumps : INVALID_HASH;
}

static void do_optimize(struct script * restrict me, const struct step_data_optimize * args)
{
    struct ofsm * restrict ofsm = me->ofsm;
    if (ofsm->qflakes <= 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Try to optmimize empty OFSM.");
        me->status = STATUS__FAILED;
        return;
    }

    unsigned int nflake = args->nflake;
    if (nflake <= 0 || nflake >= ofsm->qflakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid flake number (%u), expected value might be in range 1 - %u.", nflake, ofsm->qflakes - 1);
        me->status = STATUS__FAILED;
        return;
    }

    struct flake * restrict flake = ofsm->flakes + nflake;
    verbose(stderr, "START optimize step, nlayer  %d.", nflake);



    state_t old_qstates = flake->qstates;
    input_t qinputs = flake->qinputs;


    size_t sizes[2];
    sizes[0] = 0;
    sizes[1] = old_qstates * sizeof(struct state_info);

    void * ptrs[2];
    multialloc(2, sizes, ptrs, 32);
    void * ptr = ptrs[0];

    if (ptr == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "  multialloc(2, {%lu, %lu}, ptrs, 32) failed for temporary optimizing data.", sizes[0], sizes[1]);
        me->status = STATUS__FAILED;
        return;
    }

    struct state_info * state_infos = ptrs[1];



    { verbose(stderr, "  --> calc state hashes and sort.");

        hash_func * f = args->f != NULL ? args->f : get_first_jump;

        uint64_t counter = 0;
        double start = get_app_age();

        const state_t * jumps = flake->jumps[1];
        struct state_info * restrict ptr = state_infos;
        const struct state_info * end = state_infos + old_qstates;
        state_t state = 0;
        for (; ptr != end; ++ptr) {
            ptr->old = state++;
            ptr->hash = f(qinputs, jumps);
            jumps += qinputs;

            if ((++counter & 0xFF) == 0) {
                double now = get_app_age();
                if (now - start > 10.0) {
                    uint64_t processed = ptr - state_infos;
                    verbose(stderr, "     processed %5.2f%% (%lu of %u).", 100.0 * processed / old_qstates, processed, old_qstates);
                    start = now;
                }
            }
        }

        verbose(stderr, "        sorting...");

        qsort(state_infos, old_qstates, sizeof(struct state_info), cmp_state_info);

    } verbose(stderr, "  <<< calc state hashes and sort.");



    state_t new_qstates = 0;

    { verbose(stderr, "  --> merge states.");

        double start = get_app_age();
        uint64_t counter = 0;
        uint64_t merged = 0;

        const struct state_info * end = state_infos + old_qstates;
        struct state_info * left = state_infos;

        while (left != end) {
            struct state_info * right = left + 1;
            while (right != end && right->hash == left->hash) {
                ++right;
            }

            struct state_info * base;
            if (left->hash != INVALID_HASH || left == state_infos) {
                base = left;
                ++new_qstates;
                left->new = left->old;
                ++left;
            } else {
                base = state_infos;
            }

            while (left != right) {

                // Current state is unmerged by default
                left->new = left->old;

                // Try to merge
                for (struct state_info * ptr = base; ptr != left; ++ptr) {

                    if (ptr->new != ptr->old) {
                        // This state was merged with another one, no sence to merge.
                        continue;
                    }

                    // Try to merge
                    state_t * a = flake->jumps[1] + ptr->old * qinputs;
                    state_t * b = flake->jumps[1] + left->old * qinputs;
                    int was_merged = merge(qinputs, a, b) != 0;

                    if (was_merged) {
                        left->new = ptr->new;
                        ++merged;
                        break;
                    }
                }

                if ((++counter & 0xFFF) == 0) {
                    double now = get_app_age();
                    if (now - start > 60.0) {
                        uint64_t processed = left - state_infos;
                        uint64_t total = old_qstates;
                        double persent = 100.0 * processed / total;

                        uint64_t chunk_processed = left - base;
                        uint64_t chunk_total = right - base;
                        double chunk_persent = 100.0 * chunk_processed / chunk_total;

                        verbose(stderr, "      processed total %5.2f%%, this chunk %5.2f%%: total %lu/%lu, chunk %lu/%lu, merged = %lu.",
                            persent, chunk_persent, processed, total, chunk_processed, chunk_total, merged);

                        start = now;
                    }
                }

                new_qstates += left->new == left->old;
                ++left;
            }
        }
    } verbose(stderr, "  <<< merge states.");



    { verbose(stderr, "  --> replace old jumps with a new one.");

        state_t new_qstate = 0;

        size_t sizes[2] = { 0, new_qstates * qinputs * sizeof(state_t) };
        void * ptrs[2];
        multialloc(2, sizes, ptrs, 32);
        if (ptrs[0] == NULL) {
            ERRLOCATION(stderr);
            msg(stderr, "  multialloc(2, { %lu, %lu }, ptrs, 32) failed with NULL value during reallocating new jump table.", sizes[0], sizes[1]);
            me->status = STATUS__FAILED;
            free(ptr);
            return;
        }

        state_t * new_jumps = ptrs[1];
        const state_t * old_jumps = flake->jumps[1];
        struct state_info * restrict ptr = state_infos;
        const struct state_info * end = state_infos + old_qstates;
        for (; ptr != end; ++ptr) {
            if (ptr->new != ptr->old) {
                state_infos[ptr->old].index = state_infos[ptr->new].index;
                continue;
            }

            state_infos[ptr->old].index = new_qstate++;
            memcpy(new_jumps, old_jumps + ptr->old * qinputs, qinputs * sizeof(state_t));
            new_jumps += qinputs;
        }

        free(flake->jumps[0]);
        flake->jumps[0] = ptrs[0];
        flake->jumps[1] = ptrs[1];
        flake->qstates = new_qstates;

    } verbose(stderr, "  <<< replace old jumps with a new one.");



    { verbose(stderr, "  --> decode output states in the previous flake.");

        const struct flake * prev = flake - 1;
        state_t * restrict jump = prev->jumps[1];
        const state_t * end = jump + prev->qinputs * prev->qstates;
        for (; jump != end; ++jump) {
            if (*jump != INVALID_STATE) {
                *jump = state_infos[*jump].index;
            }
        }

    } verbose(stderr, "  <<< decode output states in the previous flake.");



    free(ptr);
    verbose(stderr, "DONE optimize step, state count was requced from %u to %u.", old_qstates, new_qstates);
}

static void do_step(struct script * restrict me)
{
    const void * args = &me->step->data;

    switch (me->step->type) {
        case STEP__GROW_POW:     return do_pow(me, args);
        case STEP__GROW_COMB:    return do_comb(me, args);
        case STEP__PACK:         return do_pack(me, args);
        case STEP__OPTIMIZE:     return do_optimize(me, args);
        default: break;
    }

    ERRLOCATION(stderr);
    msg(stderr, "Insupported step type: %d.", me->step->type);
    me->status = STATUS__FAILED;
}

static void run(struct script * restrict me)
{
    me->status = STATUS__EXECUTING;

    for (;;) {
        verbose(stderr, "New step.");

        if (me->step == NULL) {
            me->status = STATUS__DONE;
            return;
        }

        if (opt_save_steps) {
            verbose(stderr, "Start saving intermediate OFSM to file.");
            save(me);
            verbose(stderr, "DONE saving.");
        }

        do_step(me);

        if (me->status == STATUS__INTERRUPTED) {
            verbose(stderr, "Start saving current OFSM to file after interrupting.");
            save(me);
            verbose(stderr, "DONE saving.");
            return;
        }

        if (me->status == STATUS__FAILED) {
            verbose(stderr, "FAIL: current step turn status to error.");
            return;
        }

        me->step = me->step->next;
    }
}

int do_ofsm_execute(const struct ofsm * me, unsigned int n, const input_t * inputs)
{
    if (n >= me->qflakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Maximum input size is equal to qflakes = %u, but %u as passed as argument n.", me->qflakes, n);
        return -1;
    }

    for (int i=0; i<n; ++i) {
        const struct flake * flake = me->flakes + i + 1;
        if (inputs[i] >= flake->qinputs) {
            ERRLOCATION(stderr);
            msg(stderr, "Invalid input, the inputs[%d] = %u, it increases flake qinputs (%u).", i, inputs[i], flake->qinputs);
            return -1;
        }
    }

    state_t state = 0;
    const struct flake * flake = me->flakes + 1;

    for (int i=0; i<n; ++i) {
        input_t input = inputs[i];

        state = flake->jumps[1][state * flake->qinputs + input];
        if (state == INVALID_STATE) return -1;
        if (state >= flake->qoutputs) {
            ERRLOCATION(stderr);
            msg(stderr, "Invalid OFSM, new state = %u more than qoutputs = %u.", state, flake->qoutputs);
            return -1;
        }

        ++flake;
    }

    return state;
}



int do_ofsm_get_array(const struct ofsm * ofsm, unsigned int delta_last, struct ofsm_array * restrict out)
{
    memset(out, 0, sizeof(struct ofsm_array));

    if (ofsm->qflakes <= 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument: try to build and for empty OFSM.");
        return 1;
    }

    input_t qinputs_err = 0;
    for (unsigned int nflake = 0; nflake < ofsm->qflakes; ++nflake) {
        const struct flake * flake = ofsm->flakes + nflake;
        if (flake->qinputs > qinputs_err) {
             qinputs_err = flake->qinputs;
        }
    }

    if (qinputs_err == 0) {
        ERRLOCATION(stderr);
        msg(stderr, "Assertion failed: maximum input count for all flakes is 0.");
        return 1;
    }

    out->qflakes = ofsm->qflakes - 1;
    out->start_from = qinputs_err;
    out->len = qinputs_err;

    for (unsigned int nflake = 1; nflake <  ofsm->qflakes; ++nflake) {
        const struct flake * flake = ofsm->flakes + nflake;
        out->len += flake->qinputs * flake->qstates;
    }

    size_t sz = out->len * sizeof(unsigned int);
    out->array = malloc(sz);
    if (out->array == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "malloc(%lu) failed with NULL as return value in “do_ofsm_get_table”.", sz);
        return 1;
    }



    unsigned int * restrict ptr = out->array;

    for (input_t input = 0; input < qinputs_err; ++input) {
        *ptr++ = 0;
    }

    for (unsigned int nflake = 1; nflake < ofsm->qflakes - 1; ++nflake) {
        const struct flake * flake = ofsm->flakes + nflake;
        uint64_t qjumps = flake->qinputs * flake->qstates;
        const state_t * jump = flake->jumps[1];
        const state_t * end = jump + qjumps;
        uint64_t offset = ptr - out->array + qjumps;
        for (; jump != end; ++jump) {
            if (*jump == INVALID_STATE) {
                *ptr++ = 0;
            } else {
                *ptr++ = offset + *jump * flake[1].qinputs;
            }
        }
    }

    const struct flake * flake = ofsm->flakes + ofsm->qflakes - 1;
    uint64_t qjumps = flake->qinputs * flake->qstates;
    const state_t * jump = flake->jumps[1];
    const state_t * end = jump + qjumps;
    for (; jump != end; ++jump) {
        if (*jump == INVALID_STATE) {
            *ptr++ = 0;
        } else {
            *ptr++ = *jump + delta_last;
        }
    }

    return 0;
}



int ofsm_print_array(FILE * f, const char * name, const struct ofsm_array * array, unsigned int qcolumns)
{
    if (qcolumns == 0) {
        qcolumns = 30;
    }

    fprintf(f, "unsigned int %s[%lu] = {\n", name, array->len);
    const unsigned int * ptr = array->array;
    const unsigned int * end = array->array + array->len;
    int pos = 1;
    fprintf(f, "  %u",  *ptr++);
    for (; ptr != end; ++ptr) {
        const char * delimeter = (pos++ % qcolumns) == 0 ? "\n " : "";
        fprintf(f, ",%s %u", delimeter, *ptr);
    }
    fprintf(f, "\n};\n");

    return 0;
}

int ofsm_save_binary_array(FILE * f, const char * name, const struct ofsm_array * array)
{
    struct array_header header;
    memset(header.name, 0, 16);
    header.start_from = array->start_from;
    header.qflakes = array->qflakes;
    header.len = array->len;
    strncpy(header.name, name, 16);

    size_t sz = sizeof(struct array_header);
    size_t written = fwrite(&header, 1, sz, f);
    if (written != sz) {
        return 1;
    }

    sz = header.len * sizeof(uint32_t);
    written = fwrite(array->array, 1, sz, f);
    if (written != sz) {
        return 1;
    }

    return 0;
}



const input_t * do_ofsm_get_path(const struct ofsm * ofsm, unsigned int nflake, state_t output)
{
    if (nflake <= 0 || nflake >= ofsm->qflakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument “nflake” = %u, should be in range 1 - %u.", nflake, ofsm->qflakes - 1);
        return NULL;
    }

    const struct flake * flake = ofsm->flakes + nflake;

    if (output >= flake->qoutputs) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument “output” = %u for given flake, should be in range 0 - %u.", output, flake->qoutputs - 1);
        return NULL;
    }

    return flake->paths[1] + output * nflake;
}



int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check)
{
    if (build == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument “f” = NULL (build script function). Not NULL value is expected.");
        return 1;
    }

    int errcode = parse_command_line(argc, argv);
    if (opt_help || errcode != 0) {
        usage();
        return errcode;
    }

    verbose(stderr, "Start OFSM synthesis.");

    struct script * restrict script;
    script = create_script();
    if (script == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "create_script failed with NULL value. Terminate script execution.");
        return 1;
    }

    build(script);
    if (script->status == STATUS__FAILED) {
        msg(stderr, "build(script) failed. Terminate script execution.");
    }
    verbose(stderr, "DONE: building script.");

    run(script);
    verbose(stderr, "DONE: running script.");

    if (script->status == STATUS__DONE) {
        if (check != NULL) {
            errcode = check(script->ofsm);
            if (errcode != 0) {
                msg(stderr, "Check failed with code %d.", errcode);
                script->status = STATUS__FAILED;
            }
        }
    }

    int exit_code = script->status == STATUS__DONE || script->status == STATUS__INTERRUPTED ? 0 : 1;
    free_script(script);
    verbose(stderr, "Exit function execute.");
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
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct step));
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

static void add_step_pow(struct script * restrict me, input_t qinputs, unsigned int m)
{
    struct step * restrict step = me->last;
    step->type = STEP__GROW_POW;
    struct step_data_pow * restrict data = &step->data.pow;
    data->qinputs = qinputs;
    data->m = m;
}

static void add_step_comb(struct script * restrict me, input_t qinputs, unsigned int m)
{
    struct step * restrict step = me->last;
    step->type = STEP__GROW_COMB;
    struct step_data_comb * restrict data = &step->data.comb;
    data->qinputs = qinputs;
    data->m = m;
}

static void add_step_pack(struct script * restrict me, pack_func f, unsigned int flags)
{
    struct step * restrict step = me->last;
    step->type = STEP__PACK;
    struct step_data_pack * restrict data = data = &step->data.pack;
    data->f = f;
    data->flags = flags;
}

static void add_step_optimize(struct script * restrict me, unsigned int nflake, hash_func f)
{
    struct step * restrict step = me->last;
    step->type = STEP__OPTIMIZE;
    struct step_data_optimize * restrict data = data = &step->data.optimize;
    data->nflake = nflake;
    data->f = f;
}

void script_step_pow(void * restrict script, input_t qinputs, unsigned int m)
{
    if (append_step(script) != NULL) {
        add_step_pow(script, qinputs, m);
    }
}

void script_step_comb(void * restrict script, input_t qinputs, unsigned int m)
{
    if (append_step(script) != NULL) {
        add_step_comb(script, qinputs, m);
    }
}

void script_step_pack(void * restrict script, pack_func f, unsigned int flags)
{
    if (append_step(script) != NULL) {
        add_step_pack(script, f, flags);
    }
}

void script_step_optimize(void * restrict script, unsigned int nflake, hash_func f)
{
    if (append_step(script) != NULL) {
        add_step_optimize(script, nflake, f);
    }
}

void script_fail(void * restrict script)
{
    struct script * restrict me = script;
    me->status = STATUS__FAILED;
}

int ofsm_execute(const void * ofsm, unsigned int n, const input_t * inputs)
{
    return do_ofsm_execute(ofsm, n, inputs);
}

int ofsm_get_array(const void * ofsm, unsigned int delta_last, struct ofsm_array * restrict out)
{
    return do_ofsm_get_array(ofsm, delta_last, out);
}

const input_t * ofsm_get_path(const void * ofsm, unsigned int nflake, state_t output)
{
    return do_ofsm_get_path(ofsm, nflake, output);
}



struct ofsm_builder * create_ofsm_builder(struct mempool * restrict mempool, FILE * errstream)
{
    struct mempool * restrict actual_mempool = mempool;

    if (actual_mempool == NULL) {
        actual_mempool = create_mempool(4000);
        if (actual_mempool == NULL) {
            ERRLOCATION(errstream);
            msg(errstream, "create_mempool(4000) failed with NULL as return value.");
            return NULL;
        }
    }

    size_t sz = sizeof(struct ofsm_builder);
    struct ofsm_builder * restrict result = mempool_alloc(actual_mempool, sz);
    if (result == NULL) {
        ERRLOCATION(errstream);
        msg(errstream, "mempool_alloc(actual_mempool, %lu) failed with NULL as return value.", sz);
    }

    result->mempool = actual_mempool;
    result->logstream = NULL;
    result->errstream = errstream;
    result->ofsm_stack_first = 0;
    result->ofsm_stack_last = 0;
    return result;
}

void free_ofsm_builder(struct ofsm_builder * restrict me)
{
    for (size_t i=me->ofsm_stack_first; i != me->ofsm_stack_last; i = (i+1) % OFSM_STACK_SZ) {
        free_ofsm(me->ofsm_stack[i]);
    }

    free_mempool(me->mempool);
}

int ofsm_builder_push_comb(struct ofsm_builder * restrict me, input_t qinputs, unsigned int m)
{
    verbose(me->logstream, "START push compinatoric OFSM(%u, %u) to stack.", (unsigned int)qinputs, m);

    const size_t last = me->ofsm_stack_last;
    const size_t next = (last + 1) % OFSM_STACK_SZ;
    if (next == me->ofsm_stack_first) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_builder_push_comb failed, stack overflow, stack_first = %lu, stack_last = %lu, size_sz = %lu.", me->ofsm_stack_first, me->ofsm_stack_last, (size_t)OFSM_STACK_SZ);
        return 1;
    }

    struct ofsm * restrict ofsm = create_ofsm(me->mempool, 0);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "create_ofsm(me->mempool, 0) failed with NULL as result value.");
        return 1;
    }

    me->ofsm_stack[last] = ofsm;
    me->ofsm_stack_last = next;
    return 0;
}
