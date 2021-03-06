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



struct flake
{
    input_t qinputs;
    state_t qstates;
    state_t qoutputs;
    state_t * jumps[2];
    input_t * paths[2];
};

struct ofsm
{
    unsigned int qflakes;
    unsigned int max_flakes;
    struct flake * flakes;
};

struct ofsm_pack_decode {
    state_t output;
    pack_value_t value;
};

struct state_info
{
    state_t old;
    state_t new;
    uint64_t hash;
    state_t index;
};



static const struct flake zero_flake = { 0, 0, 1, { NULL, NULL }, { NULL, NULL } };



/* Sort order */

static int cmp_ofsm_pack_decode(const void * const arg_a, const void * const arg_b)
{
    const struct ofsm_pack_decode * const a = arg_a;
    const struct ofsm_pack_decode * const b = arg_b;
    if (a->value < b->value) return -1;
    if (a->value > b->value) return +1;
    if (a->output < b->output) return -1;
    if (a->output > b->output) return +1;
    return 0;
}

static int cmp_state_info(const void * const arg_a, const void * const arg_b)
{
    const struct state_info * const a = arg_a;
    const struct state_info * const b = arg_b;
    if (a->hash < b->hash) return -1;
    if (a->hash > b->hash) return +1;
    if (a->old < b->old) return -1;
    if (a->old > b->old) return +1;
    return 0;
}



/* Utils */

static int calc_paths(const struct flake * const flake, const unsigned int nflake)
{
    const input_t qinputs = flake->qinputs;
    const state_t qstates = flake->qstates;
    const uint64_t qoutputs = flake->qoutputs;

    {
        // May be not required, this is just optimization.
        // To catch errors we fill data

        input_t * restrict ptr = flake->paths[1];
        const input_t * const end = ptr + nflake * qoutputs;
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

    const struct flake * const prev = flake - 1;
    const state_t * data_ptr = flake->jumps[1];

    if (nflake > 1) {

        // Common version
        for (state_t state=0; state<qstates; ++state) {
            const input_t * const base = prev->paths[1] + state * (nflake-1);
            for (input_t input=0; input < qinputs; ++input) {
                const state_t output = *data_ptr++;
                if (output == INVALID_STATE) continue;
                input_t * restrict const ptr = flake->paths[1] + output * nflake;
                memcpy(ptr, base, (nflake-1) * sizeof(input_t));
                ptr[nflake-1] = input;
            }
        }

    } else {

        // Optimized version, only one state, no copying previous paths
        for (input_t input=0; input < qinputs; ++input) {
            const state_t output = *data_ptr++;
            input_t * restrict const ptr = flake->paths[1] + output;
            *ptr = input;
        }
    }

    return 0;
}



/* OFSM methods */

static struct ofsm * create_ofsm(struct mempool * restrict const mempool, const unsigned int arg_max_flakes)
{
    const unsigned int max_flakes = arg_max_flakes != 0 ? arg_max_flakes : 32;

    struct ofsm * restrict const ofsm = mempool_alloc(mempool, sizeof(struct ofsm));
    if (ofsm == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sizeof(struct ofsm));
        return NULL;
    }

    const size_t sz = max_flakes * sizeof(struct flake);
    struct flake * restrict const flakes = mempool_alloc(mempool, sz);
    if (flakes == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "mempool_alloc(mempool, %lu) failed with NULL as return value.", sz);
        return NULL;
    }

    ofsm->qflakes = 1;
    ofsm->max_flakes = max_flakes;
    ofsm->flakes = flakes;

    const size_t flake_sz = sizeof(struct flake);
    memcpy(ofsm->flakes, &zero_flake, flake_sz);
    return ofsm;
}

static void ofsm_truncate(struct ofsm * restrict const me, const unsigned int qflakes)
{
    for (unsigned int i=qflakes; i<me->qflakes; ++i) {

        void * const jump_ptr = me->flakes[i].jumps[0];
        if (jump_ptr != NULL) {
            free(jump_ptr);
        }

        void * const path_ptr = me->flakes[i].paths[0];
        if (path_ptr) {
            free(path_ptr);
        }
    }

    me->qflakes = qflakes;
}

static void free_ofsm(struct ofsm * restrict me)
{
    ofsm_truncate(me, 1);
}



static struct flake * ofsm_create_flake(struct ofsm * restrict const ofsm, input_t qinputs, const uint64_t qoutputs, const state_t qstates)
{
    const unsigned int nflake = ofsm->qflakes;
    if (nflake >= ofsm->max_flakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Overflow maximum flake count (%u), qflakes = %u.", ofsm->max_flakes, ofsm->qflakes);
        return NULL;
    }

    void * jump_ptrs[2];
    void * path_ptrs[2];
    const size_t jump_sizes[2] = { 0, qinputs * qstates * sizeof(state_t) };
    const size_t path_sizes[2] = { 0, qoutputs * nflake * sizeof(input_t) };

    multialloc(2, jump_sizes, jump_ptrs, 32);

    if (jump_ptrs[0] == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "multialloc(2, {%lu, %lu}, ptrs, 32) failed for new flake.", jump_sizes[0], jump_sizes[1]);
        return NULL;
    }

    multialloc(2, path_sizes, path_ptrs, 32);

    if (path_ptrs[0] == NULL) {
        ERRLOCATION(stderr);
        msg(stderr, "multialloc(2, {%lu, %lu}, ptrs, 32) failed for new flake.", path_sizes[0], path_sizes[1]);
        free(jump_ptrs[0]);
        return NULL;
    }

    struct flake * restrict const flake = ofsm->flakes + nflake;

    flake->qinputs = qinputs;
    flake->qoutputs = qoutputs;
    flake->qstates = qstates;
    flake->jumps[0] = jump_ptrs[0];
    flake->jumps[1] = jump_ptrs[1];
    flake->paths[0] = path_ptrs[0];
    flake->paths[1] = path_ptrs[1];

    ++ofsm->qflakes;
    return flake;
}



static state_t do_ofsm_execute(const struct ofsm * const me, const unsigned int n, const input_t * const inputs)
{
    if (n >= me->qflakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Maximum input size is equal to qflakes = %u, but %u as passed as argument n.", me->qflakes, n);
        return -1;
    }

    for (int i=0; i<n; ++i) {
        const struct flake * const flake = me->flakes + i + 1;
        if (inputs[i] >= flake->qinputs) {
            ERRLOCATION(stderr);
            msg(stderr, "Invalid input, the inputs[%d] = %u, it increases flake qinputs (%u).", i, inputs[i], flake->qinputs);
            return -1;
        }
    }

    state_t state = 0;
    const struct flake * flake = me->flakes + 1;

    for (int i=0; i<n; ++i) {
        const input_t input = inputs[i];

        state = flake->jumps[1][state * flake->qinputs + input];
        if (state == INVALID_STATE) return INVALID_STATE;
        if (state >= flake->qoutputs) {
            ERRLOCATION(stderr);
            msg(stderr, "Invalid OFSM, new state = %u more than qoutputs = %u.", state, flake->qoutputs);
            return INVALID_STATE;
        }

        ++flake;
    }

    return state;
}



static int do_ofsm_get_array(const struct ofsm * const ofsm, const unsigned int delta_last, struct ofsm_array * restrict const out)
{
    memset(out, 0, sizeof(struct ofsm_array));

    if (ofsm->qflakes <= 1) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument: try to build and for empty OFSM.");
        return 1;
    }

    input_t qinputs_err = 0;
    for (unsigned int nflake = 0; nflake < ofsm->qflakes; ++nflake) {
        const struct flake * const flake = ofsm->flakes + nflake;
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
        const struct flake * const flake = ofsm->flakes + nflake;
        out->len += flake->qinputs * flake->qstates;
    }

    const size_t sz = out->len * sizeof(unsigned int);
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
        const struct flake * const flake = ofsm->flakes + nflake;
        const uint64_t qjumps = flake->qinputs * flake->qstates;
        const state_t * jump = flake->jumps[1];
        const state_t * const end = jump + qjumps;
        const uint64_t offset = ptr - out->array + qjumps;
        for (; jump != end; ++jump) {
            if (*jump == INVALID_STATE) {
                *ptr++ = 0;
            } else {
                *ptr++ = offset + *jump * flake[1].qinputs;
            }
        }
    }

    const struct flake * const flake = ofsm->flakes + ofsm->qflakes - 1;
    const uint64_t qjumps = flake->qinputs * flake->qstates;
    const state_t * jump = flake->jumps[1];
    const state_t * const end = jump + qjumps;
    for (; jump != end; ++jump) {
        if (*jump == INVALID_STATE) {
            *ptr++ = 0;
        } else {
            *ptr++ = *jump + delta_last;
        }
    }

    return 0;
}



static const input_t * do_ofsm_get_path(const struct ofsm * const ofsm, const unsigned int nflake, const state_t output)
{
    if (nflake <= 0 || nflake >= ofsm->qflakes) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument “nflake” = %u, should be in range 1 - %u.", nflake, ofsm->qflakes - 1);
        return NULL;
    }

    const struct flake * const flake = ofsm->flakes + nflake;

    if (output >= flake->qoutputs) {
        ERRLOCATION(stderr);
        msg(stderr, "Invalid argument “output” = %u for given flake, should be in range 0 - %u.", output, flake->qoutputs - 1);
        return NULL;
    }

    return flake->paths[1] + output * nflake;
}



static int ofsm_verify(const struct ofsm * const me, FILE * const errstream)
{
    const size_t flake_sz = sizeof(struct flake);
    if (memcmp(me->flakes, &zero_flake, flake_sz) != 0) {
        ERRLOCATION(errstream);
        msg(errstream, "Verification failed: invalid zero flake.\n");
        return 1;
    }

    for (unsigned int nflake = 1; nflake < me->qflakes; ++nflake) {
        const struct flake * const flake = me->flakes + nflake;
        const struct flake * const prev = flake -1;
        if (prev->qoutputs != flake->qstates) {
            ERRLOCATION(errstream);
            msg(errstream, "Verification failed: Mismatch flake->qstates = %u and prev->qoutputs = %u.\n", flake->qstates, prev->qoutputs);
            return 1;
        }

        const state_t qoutputs = flake->qoutputs;

        const state_t * jump = flake->jumps[1];
        const state_t * const jump_last = jump + flake->qinputs * flake->qstates;
        for (; jump != jump_last; ++jump) {
            const state_t state = *jump;
            if (state > qoutputs && state != INVALID_STATE) {
                ERRLOCATION(errstream);
                msg(errstream, "Verification failed: invalid state %u\n", state);
                return 1;
            }
        }

        const input_t * input = flake->paths[1];

        for (state_t output=0; output<qoutputs; ++output) {

            int is_invalid = 1;
            for (unsigned int i=0; i<nflake; ++i) {
                if (input[i] != INVALID_INPUT) {
                    is_invalid = 0;
                    break;
                }
            }

            if (is_invalid) {
                continue;
            }

            for (unsigned int i=1; i<=nflake; ++i) {
                const struct flake * const current_flake = me->flakes + i;
                if (input[i-1] >= current_flake->qinputs) {
                    ERRLOCATION(errstream);
                    msg(errstream, "Verification failed: in nflake %u invalid state input[%u] = %u, qinputs = %u.", nflake, i-1, input[i-1], current_flake->qinputs);
                    fprintf(errstream, "Input:");
                    for (unsigned int j=0; j<nflake; ++j) {
                        fprintf(errstream, " %u", input[j]);
                    }
                    fprintf(errstream, "\n");
                    return 1;
                }
            }

            const state_t state = ofsm_execute(me, nflake, input);
            if (state != output) {
                ERRLOCATION(errstream);
                msg(errstream, "Verification failed: execution from path does not lead to output state.");
                msg(errstream, "Output = %u, state = %u.", output, state);
                fprintf(errstream, "Input:");
                for (unsigned int j=0; j<nflake; ++j) {
                    fprintf(errstream, " %u", input[j]);
                }
                fprintf(errstream, "\n");

                return 1;
            }

            input += nflake;
        }

    }

    return 0;
}



static struct ofsm * do_ofsm_builder_get_ofsm(const struct ofsm_builder * const me)
{
    if (me->stack_len == 0) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "do_ofsm_builder_get_ofsm is called for empty OFSM stack.");
        return NULL;
    }

    return me->stack[me->stack_len - 1];
}



/* OFSM Builder */

static int autoverify(const struct ofsm_builder * const me)
{
    return (me->flags & OBF__AUTO_VERIFY) ? ofsm_builder_verify(me) : 0;
}



struct ofsm_builder * create_ofsm_builder(struct mempool * restrict const arg_mempool, FILE * const errstream)
{
    struct mempool * restrict mempool = arg_mempool != NULL ? arg_mempool : create_mempool(4000);

    if (mempool == NULL) {
        ERRLOCATION(errstream);
        msg(errstream, "create_mempool(4000) failed with NULL as return value.");
        return NULL;
    }

    const size_t sz = sizeof(struct ofsm_builder);
    struct ofsm_builder * restrict const result = mempool_alloc(mempool, sz);
    if (result == NULL) {
        ERRLOCATION(errstream);
        msg(errstream, "mempool_alloc(actual_mempool, %lu) failed with NULL as return value.", sz);
    }

    result->flags = 0;
    if (mempool != arg_mempool) {
        result->flags |= OBF__OWN_MEMPOOL;
    }

    result->mempool = mempool;
    result->logstream = NULL;
    result->errstream = errstream;
    result->stack_len = 0;
    result->user_data = NULL;
    init_choose_table(&result->choose, 0, 0, errstream);
    return result;
}



void free_ofsm_builder(struct ofsm_builder * restrict const me)
{
    clear_choose_table(&me->choose);

    for (unsigned int i = 0; i < me->stack_len; ++i) {
        free_ofsm(me->stack[i]);
    }

    if (me->flags & OBF__OWN_MEMPOOL) {
        free_mempool(me->mempool);
    }
}



int ofsm_builder_make_array(const struct ofsm_builder * const me, const unsigned int delta_last, struct ofsm_array * restrict const out)
{
    const void * const ofsm = do_ofsm_builder_get_ofsm(me);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "do_ofsm_builder_get_ofsm_as_void(me) failed with NULL as error value.");
        return 1;
    }

    return ofsm_get_array(ofsm, delta_last, out);
}



int ofsm_builder_push_pow(struct ofsm_builder * restrict const me, const input_t qinputs, const unsigned int m)
{
    verbose(me->logstream, "START push power OFSM(%u, %u) to stack.", (unsigned int)qinputs, m);

    if (me->stack_len == OFSM_STACK_SZ) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_builder_push_pow failed, stack overflow, stack_len = %u, size_sz = %u.", me->stack_len, OFSM_STACK_SZ);
        verbose(me->logstream, "FAILED push power.");
        return 1;
    }

    struct ofsm * restrict const ofsm = create_ofsm(me->mempool, 0);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "create_ofsm(me->mempool, 0) failed with NULL as result value.");
        verbose(me->logstream, "FAILED push power.");
        return 1;
    }

    const struct flake * prev =  ofsm->flakes + ofsm->qflakes - 1;

    for (unsigned int i=0; i<m; ++i) {
        if (prev->qoutputs == INVALID_STATE) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "state_t overflow: try to assign %u to qstates.", prev->qoutputs);
            verbose(me->logstream, "FAILED push power.");
            free_ofsm(ofsm);
            return 1;
        }

        const state_t qstates = prev->qoutputs;
        const uint64_t qoutputs = qstates * qinputs;

        const unsigned int nflake = ofsm->qflakes;
        const struct flake * const flake = ofsm_create_flake(ofsm, qinputs, qoutputs, qstates);

        if (flake == NULL) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "ofsm_create_flake(me, %u, %lu, %u) faled with NULL as return value.", qinputs, qoutputs, qstates);
            verbose(me->logstream, "FAILED push power.");
            free_ofsm(ofsm);
            return 1;
        }

        state_t * restrict jumps = flake->jumps[1];
        state_t output = 0;

        for (uint64_t state=0; state<qstates; ++state)
        for (uint64_t input=0; input<qinputs; ++input) {
            *jumps++ = output++;
        }

        const int status = calc_paths(flake, nflake);
        if (status != 0) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "calc_paths(flake, %u) failed with %d as an error code.", nflake, status);
            verbose(me->logstream, "FAILED push power.");
            free_ofsm(ofsm);
            return 1;
        }

        prev = flake;
    }

    me->stack[me->stack_len++] = ofsm;
    verbose(me->logstream, "DONE push power.");
    return autoverify(me);
}



static state_t calc_comb_index(const struct choose_table * const ct, const input_t * const inputs, unsigned int qinputs, const unsigned int m)
{

    if (qinputs > 64) {
        ERRLOCATION(ct->errstream);
        msg(ct->errstream, "Sorting of inputs more then 64 is not supported yet.");
        return INVALID_STATE;
    }

    uint64_t mask = 0;
    for (unsigned int i=0; i<m; ++i) {
        const uint64_t new_one = 1ull << inputs[i];
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

int ofsm_builder_push_comb(struct ofsm_builder * restrict const me, const input_t qinputs, const unsigned int m)
{
    verbose(me->logstream, "START push compinatoric OFSM(%u, %u) to stack.", (unsigned int)qinputs, m);

    if (me->stack_len == OFSM_STACK_SZ) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_builder_push_comb failed, stack overflow, stack_len = %u, size_sz = %u.", me->stack_len, OFSM_STACK_SZ);
        verbose(me->logstream, "FAILED push combinatoric.");
        return 1;
    }

    struct choose_table * restrict const ct = &me->choose;
    const int status = rebuild_choose_table(ct, qinputs, m);
    if (status != 0) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "rebuild_choose_table(ct, %u, %u, errstream) failed with %d as error code.", (unsigned int)qinputs, m, status);
        verbose(me->logstream, "FAILED push combinatoric.");
        return status;
    }

    struct ofsm * restrict const ofsm = create_ofsm(me->mempool, 0);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "create_ofsm(me->mempool, 0) failed with NULL as result value.");
        verbose(me->logstream, "FAILED push combinatoric.");
        return 1;
    }

    const struct flake * prev =  ofsm->flakes + ofsm->qflakes - 1;

    uint64_t nn = qinputs;
    uint64_t dd = 1;

    for (unsigned int i=0; i<m; ++i) {
        if (prev->qoutputs == INVALID_STATE) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "state_t overflow: try to assign %u to qstates.", prev->qoutputs);
            verbose(me->logstream, "FAILED push combinatoric.");
            free_ofsm(ofsm);
            return 1;
        }

        const state_t qstates = prev->qoutputs;
        const uint64_t qoutputs = qstates * nn / dd;

        unsigned int nflake = ofsm->qflakes;
        const struct flake * flake = ofsm_create_flake(ofsm, qinputs, qoutputs, qstates);
        if (flake == NULL) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "ofsm_create_flake(me, %u, %lu, %u) faled with NULL as return value.", qinputs, qoutputs, qstates);
            verbose(me->logstream, "FAILED push combinatoric.");
            free_ofsm(ofsm);
            return 1;
        }

        state_t * restrict jumps = flake->jumps[1];

        if (i > 0) {
            for (state_t state = 0; state < qstates; ++state) {
                input_t c[i+1];
                const input_t * const ptr = prev->paths[1] + (nflake-1) * (state+1) - i;
                memcpy(c, ptr, i * sizeof(input_t));

                for (input_t input = 0; input < qinputs; ++input) {
                    c[i] = input;
                    *jumps++ = calc_comb_index(ct, c, qinputs, i+1);
                }
            }
        } else {
            if (qstates != 1) {
                ERRLOCATION(me->errstream);
                msg(me->errstream, "Internal error: zero flake should return one output! But qstates = %u.", qstates);
                verbose(me->logstream, "FAILED push combinatoric.");
                free_ofsm(ofsm);
                return 1;
            }

            for (input_t input = 0; input < qinputs; ++input) {
                *jumps++ = input;
            }
        }

        const int status = calc_paths(flake, nflake);
        if (status != 0) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "calc_paths(flake, %u) failed with %d as an error code.", nflake, status);
            verbose(me->logstream, "FAILED push combinatoric.");
            free_ofsm(ofsm);
            return 1;
        }

        prev = flake;
        --nn;
        ++dd;
    }

    me->stack[me->stack_len++] = ofsm;
    verbose(me->logstream, "DONE push combinatoric.");
    return autoverify(me);
}



int ofsm_builder_product(struct ofsm_builder * restrict const me)
{
    verbose(me->logstream, "START product.");

    if (me->stack_len < 2) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_builder_product(me) is called for too small OFSM stack, stack_len = %u.", me->stack_len);
        verbose(me->logstream, "FAILED product.");
        return 1;
    }

    struct ofsm * restrict const ofsm1 = me->stack[me->stack_len - 2];
    struct ofsm * restrict const ofsm2 = me->stack[me->stack_len - 1];

    const unsigned int saved_qflakes1 = ofsm1->qflakes;
    const struct flake * const last1 = ofsm1->flakes + saved_qflakes1 - 1;

    for (int nflake2 = 1; nflake2 < ofsm2->qflakes; ++nflake2) {
        const struct flake * const flake2 = ofsm2->flakes + nflake2;
        struct flake * restrict const flake1 = ofsm_create_flake(ofsm1, flake2->qinputs, flake2->qoutputs * last1->qoutputs, flake2->qstates * last1->qoutputs);
        if (flake1 == NULL) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "ofsm_create_flake(ofsm1, %u, %u, %u) failed with NULL as result.", flake2->qinputs, flake2->qoutputs * last1->qoutputs, flake2->qstates * last1->qoutputs);
            verbose(me->logstream, "FAILED product.");
            ofsm_truncate(ofsm1, saved_qflakes1);
            return 1;
        }

        state_t * jump1 = flake1->jumps[1];
        for (unsigned int output1 = 0; output1 < last1->qoutputs; ++output1) {
            const state_t * jump2 = flake2->jumps[1];
            for (unsigned int state2 = 0; state2 < flake2->qstates; ++state2)
            for (unsigned int input2 = 0; input2 < flake2->qinputs; ++input2) {
                if (*jump2 == INVALID_STATE) {
                    *jump1 = INVALID_STATE;
                } else {
                    *jump1 = (*jump2) + output1 * flake2->qoutputs;
                }
                ++jump1;
                ++jump2;
            }
        }

        input_t * path1 = flake1->paths[1];
        const unsigned int head_len = saved_qflakes1 - 1;
        const unsigned int tail_len = nflake2;

        const input_t * head = last1->paths[1];
        for (unsigned int output1 = 0; output1 < last1->qoutputs; ++output1) {
            const input_t * tail = flake2->paths[1];
            for (unsigned int output2 = 0; output2 < flake2->qoutputs; ++output2) {
                if (head_len > 0) {
                    memcpy(path1, head, head_len * sizeof(input_t));
                    path1 += head_len;
                }
                memcpy(path1, tail, tail_len * sizeof(input_t));
                path1 += tail_len;
                tail += tail_len;
            }
            head += head_len;
        }
    }

    free_ofsm(ofsm2);
    --me->stack_len;

    verbose(me->logstream, "DONE product.");
    return autoverify(me);
}



int ofsm_builder_pack(struct ofsm_builder * restrict const me, pack_func f, const unsigned int flags)
{
    verbose(me->logstream, "START packing.");

    struct ofsm * restrict const ofsm = do_ofsm_builder_get_ofsm(me);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "do_ofsm_builder_get_ofsm(me) failed with NULL as error value.");
        verbose(me->logstream, "FAILED packing.");
        return 1;
    }

    if (ofsm->qflakes <= 1) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "Try to pack empty OFSM.");
        verbose(me->logstream, "FAILED packing.");
        return 1;
    }

    const int skip_renumering = (flags & PACK_FLAG__SKIP_RENUMERING) != 0;
    const unsigned int nflake = ofsm->qflakes - 1;
    const struct flake oldman = ofsm->flakes[nflake];
    const uint64_t old_qoutputs = oldman.qoutputs;

    const size_t sizes[3] = { 0,
        (1 + old_qoutputs) * sizeof(struct ofsm_pack_decode),
        old_qoutputs * sizeof(state_t),
    };

    void * ptrs[3];
    multialloc(3, sizes, ptrs, 32);
    void * const ptr = ptrs[0];

    if (ptr == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "  multialloc(4, {%lu, %lu, %lu}, ptrs, 32) failed for temporary packing data.", sizes[0], sizes[1], sizes[2]);
        verbose(me->logstream, "FAILED packing.");
        return 1;
    }

    struct ofsm_pack_decode * const decode_table = ptrs[1];
    state_t * restrict const translate = ptrs[2];



    pack_value_t max_value = 0;

    { verbose(me->logstream, "  --> calculate pack values.");

        struct ofsm_pack_decode * restrict curr = decode_table;
        const input_t * path = oldman.paths[1];
        for (size_t output = 0; output < old_qoutputs; ++output) {
            curr->output = output;
            curr->value = f(me->user_data, nflake, path);
            if (curr->value > max_value) max_value = curr->value;
            path += nflake;
            ++curr;
        }

        curr->output = INVALID_STATE;
        curr->value = INVALID_PACK_VALUE;

    } verbose(me->logstream, "  <<< calculate pack values, max value is %lu (0x%lx).", max_value, max_value);



    { verbose(me->logstream, "  --> sort new states.");
        qsort(decode_table, old_qoutputs, sizeof(struct ofsm_pack_decode), &cmp_ofsm_pack_decode);
    } verbose(me->logstream, "  <<< sort new states.");



    state_t new_qoutputs = 0;

    if (skip_renumering) {

        verbose(me->logstream, "  --> calc output_translate table without renumering.");

        new_qoutputs = max_value + 1;

        const struct ofsm_pack_decode * left = decode_table;
        const struct ofsm_pack_decode * const end = decode_table + old_qoutputs;

        while (left != end) {

            const struct ofsm_pack_decode * right = left + 1;
            while (right != end && right->value == left->value) {
                ++right;
            }

            const state_t new_output = left->value != INVALID_PACK_VALUE ? left->value : INVALID_STATE;
            for (; left != right; ++left) {
                translate[left->output] = new_output;
            }
        }

        verbose(me->logstream, "  <<< calc output_translate table without renumering.");

    } else {

        verbose(me->logstream, "  --> calc output_translate table with renumering.");

        const struct ofsm_pack_decode * left = decode_table;
        const struct ofsm_pack_decode * const end = decode_table + old_qoutputs;

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

        verbose(me->logstream, "  <<< calc output_translate table with renumering.");
    }



    --ofsm->qflakes;
    struct flake * restrict const infant = ofsm_create_flake(ofsm, oldman.qinputs, new_qoutputs, oldman.qstates);
    if (infant == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_create_flake(me, %u, %u, %u) faled with NULL as return value in pack step.", oldman.qinputs, new_qoutputs, oldman.qstates);
        verbose(me->logstream, "FAILED packing.");
        free(ptr);
        return 1;
    }



    { verbose(me->logstream, "  --> update data.");

        const state_t * old = oldman.jumps[1];
        const state_t * const end = old + oldman.qstates * oldman.qinputs;
        state_t * restrict new = infant->jumps[1];

        for (; old != end; ++old) {
            if (*old != INVALID_STATE) {
                *new++ = translate[*old];
            } else {
                *new++ = INVALID_STATE;
            }
        }

    } verbose(me->logstream, "  <<< update data.");



    { verbose(me->logstream, "  --> update paths.");

        input_t * restrict new_path = infant->paths[1];
        const input_t * const old_paths = oldman.paths[1];
        const struct ofsm_pack_decode * decode = decode_table;
        const size_t sz = sizeof(input_t) * nflake;
        for (state_t output = 0; output < new_qoutputs; ++output) {
            const pack_value_t value = decode->value;
            const state_t old_output = decode->output;
            const state_t new_output = translate[old_output];
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

    } verbose(me->logstream, "  <<< update paths.");



    free(oldman.jumps[0]);
    free(oldman.paths[0]);
    free(ptr);
    verbose(me->logstream, "DONE pack step, new qoutputs = %u.", new_qoutputs);

    return autoverify(me);
}



static uint64_t get_first_jump(void * const user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * const path)
{
    return *jumps != INVALID_STATE ? *jumps : INVALID_HASH;
}

static int merge(const unsigned int qinputs, state_t * const restrict a, state_t * restrict const b)
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

static int ofsm_builder_optimize_flake(struct ofsm_builder * restrict const me, const unsigned int nflake, struct flake * restrict const flake, hash_func * f)
{
    hash_func * const hash = f != NULL ? f : get_first_jump;
    const state_t old_qstates = flake->qstates;
    const input_t qinputs = flake->qinputs;


    const size_t sizes[2] = { 0, old_qstates * sizeof(struct state_info) };

    void * ptrs[2];
    multialloc(2, sizes, ptrs, 32);
    void * const ptr = ptrs[0];

    if (ptr == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "multialloc(2, {%lu, %lu}, ptrs, 32) failed for temporary optimizing data.", sizes[0], sizes[1]);
        return 1;
    }

    struct state_info * const state_infos = ptrs[1];



    { verbose(me->logstream, "  --> calc state hashes and sort.");

        uint64_t counter = 0;
        double start = get_app_age();

        const struct flake * const prev_flake = flake - 1;
        if (prev_flake->qoutputs != old_qstates) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "Previous flack outputs %u is not match to current flake states %u.", prev_flake->qoutputs, old_qstates);
            return 1;
        }

        const state_t * jumps = flake->jumps[1];
        struct state_info * restrict ptr = state_infos;
        const struct state_info * const end = state_infos + old_qstates;
        const input_t * path = prev_flake->paths[1];
        const unsigned int path_len = nflake - 1;
        state_t state = 0;
        for (; ptr != end; ++ptr) {
            ptr->old = state++;
            ptr->hash = hash(me->user_data, qinputs, jumps, path_len, path);
            jumps += qinputs;
            path += path_len;

            if ((++counter & 0xFF) == 0) {
                const double now = get_app_age();
                if (now - start > 10.0) {
                    const uint64_t processed = ptr - state_infos;
                    verbose(me->logstream, "    processed %5.2f%% (%lu of %u).", 100.0 * processed / old_qstates, processed, old_qstates);
                    start = now;
                }
            }
        }

        verbose(me->logstream, "    sorting...");

        qsort(state_infos, old_qstates, sizeof(struct state_info), cmp_state_info);

    } verbose(me->logstream, "  <<< calc state hashes and sort.");



    state_t new_qstates = 0;

    { verbose(me->logstream, "  --> merge states.");

        double start = get_app_age();
        uint64_t counter = 0;
        uint64_t merged = 0;

        const struct state_info * const end = state_infos + old_qstates;
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
                for (const struct state_info * ptr = base; ptr != left; ++ptr) {

                    if (ptr->new != ptr->old) {
                        // This state was merged with another one, no sence to merge.
                        continue;
                    }

                    // Try to merge
                    state_t * const a = flake->jumps[1] + ptr->old * qinputs;
                    state_t * const b = flake->jumps[1] + left->old * qinputs;
                    const int was_merged = merge(qinputs, a, b) != 0;

                    if (was_merged) {
                        left->new = ptr->new;
                        ++merged;
                        break;
                    }
                }

                if ((++counter & 0xFFF) == 0) {
                    const double now = get_app_age();
                    if (now - start > 60.0) {
                        const uint64_t processed = left - state_infos;
                        const uint64_t total = old_qstates;
                        const double persent = 100.0 * processed / total;

                        const uint64_t chunk_processed = left - base;
                        const uint64_t chunk_total = right - base;
                        const double chunk_persent = 100.0 * chunk_processed / chunk_total;

                        verbose(me->logstream, "    processed total %5.2f%%, this chunk %5.2f%%: total %lu/%lu, chunk %lu/%lu, merged = %lu.",
                            persent, chunk_persent, processed, total, chunk_processed, chunk_total, merged);

                        start = now;
                    }
                }

                new_qstates += left->new == left->old;
                ++left;
            }
        }
    } verbose(me->logstream, "  <<< merge states.");



    { verbose(me->logstream, "  --> replace old jumps with a new one.");

        state_t new_qstate = 0;

        size_t sizes[2] = { 0, new_qstates * qinputs * sizeof(state_t) };
        void * ptrs[2];
        multialloc(2, sizes, ptrs, 32);
        if (ptrs[0] == NULL) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "multialloc(2, { %lu, %lu }, ptrs, 32) failed with NULL value during reallocating new jump table.", sizes[0], sizes[1]);
            free(ptr);
            return 1;
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

    } verbose(me->logstream, "  <<< replace old jumps with a new one.");



    { verbose(me->logstream, "  --> decode output states in the previous flake.");

        const struct flake * prev = flake - 1;
        state_t * restrict jump = prev->jumps[1];
        const state_t * end = jump + prev->qinputs * prev->qstates;
        for (; jump != end; ++jump) {
            if (*jump != INVALID_STATE) {
                *jump = state_infos[*jump].index;
            }
        }

        flake[-1].qoutputs = flake->qstates;

    } verbose(me->logstream, "  <<< decode output states in the previous flake.");

    if (nflake > 1) {

        verbose(me->logstream, "  --> update path from previous flake.");

        const struct flake * const prev = flake - 1;
        const size_t path_len = nflake - 1;
        const size_t new_path_len = prev->qoutputs * path_len;
        const size_t new_path_sizes[2] = { 0, new_path_len * sizeof(input_t) };
        void * new_path_ptrs[2];
        multialloc(2, new_path_sizes, new_path_ptrs, 32);

        if (new_path_ptrs[0] == NULL) {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "multialloc(2, { %lu, %lu }, ptrs, 32) failed with NULL value during reallocating new jump table.", new_path_sizes[0], new_path_sizes[1]);
            free(ptr);
            return 1;
        }

        input_t * restrict new_path = new_path_ptrs[1];
        for (size_t i=0; i<new_path_len; ++i) {
            new_path[i] = INVALID_INPUT;
        }

        const size_t path_sz = path_len * sizeof(input_t);
        for (size_t old_state = 0; old_state < old_qstates; ++old_state) {
            const state_t index = state_infos[old_state].index;
            const size_t new_path_index = index * path_len;
            const size_t old_path_index = old_state * path_len;
            if (new_path[new_path_index] == INVALID_INPUT) {
                void * new_input = new_path + new_path_index;
                const void * old_input = prev->paths[1] + old_path_index;
                memcpy(new_input, old_input, path_sz);
            }
        }

        free(flake[-1].paths[0]);
        flake[-1].paths[0] = new_path_ptrs[0];
        flake[-1].paths[1] = new_path_ptrs[1];

        verbose(me->logstream, "  <<< update path from previous flake.");
    }

    free(ptr);
    return 0;
}

int ofsm_builder_optimize(struct ofsm_builder * restrict const me, const unsigned int nflake, unsigned int qflakes, hash_func f)
{
    struct ofsm * restrict const ofsm = do_ofsm_builder_get_ofsm(me);
    if (ofsm == NULL) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "ofsm_builder_optimize(me) failed with NULL as error value.");
        return 1;
    }

    if (nflake <= 0 || nflake >= ofsm->qflakes) {
        ERRLOCATION(me->errstream);
        msg(me->errstream, "Invalid flake number (%u), expected value might be in range 1 - %u.", nflake, ofsm->qflakes - 1);
        return 1;
    }

    if (qflakes == 0) --qflakes;

    for (unsigned int i = 0; i < qflakes; ++i) {
        const unsigned int current_nflake = nflake - i;
        if (current_nflake <= 0) {
            break;
        }

        struct flake * restrict const flake = ofsm->flakes + current_nflake;
        verbose(me->logstream, "START optimize flake %u.", current_nflake);

        const int status = ofsm_builder_optimize_flake(me, current_nflake, flake, f);
        if (status == 0) {
            verbose(me->logstream, "DONE optimize flake %u.", current_nflake);
        } else {
            ERRLOCATION(me->errstream);
            msg(me->errstream, "ofsm_builder_optimize_flake(me, flake, f) failed with %d as error code.\n", status);
            verbose(me->logstream, "FAILED optimize flake %u.", current_nflake);
            return 1;
        }
    }

    return autoverify(me);
}



int ofsm_builder_verify(const struct ofsm_builder * const me)
{
    verbose(me->logstream, "START verification.");

    for (size_t i=0; i < me->stack_len; ++i) {
        const struct ofsm * const ofsm= me->stack[i];
        const int status = ofsm_verify(ofsm, me->errstream);
        if (status != 0) {
            return status;
        }
    }

    verbose(me->logstream, "DONE verification.");
    return 0;
}



const void * ofsm_builder_get_ofsm(const struct ofsm_builder * const me)
{
    return do_ofsm_builder_get_ofsm(me);
}

const input_t * ofsm_get_path(const void * const ofsm, const unsigned int nflake, const state_t output)
{
    return do_ofsm_get_path(ofsm, nflake, output);
}

state_t ofsm_execute(const void * const ofsm, const unsigned int n, const input_t * const inputs)
{
    return do_ofsm_execute(ofsm, n, inputs);
}

int ofsm_get_array(const void * const ofsm, const unsigned int delta_last, struct ofsm_array * restrict const out)
{
    return do_ofsm_get_array(ofsm, delta_last, out);
}



int ofsm_array_print(const struct ofsm_array * const array, FILE * const f, const char * const name, const unsigned int arg_qcolumns)
{
    const unsigned int qcolumns = arg_qcolumns != 0 ? arg_qcolumns : 30;

    fprintf(f, "unsigned int %s[%lu] = {\n", name, array->len);
    const unsigned int * ptr = array->array;
    const unsigned int * const end = array->array + array->len;
    int pos = 1;
    fprintf(f, "  %u",  *ptr++);
    for (; ptr != end; ++ptr) {
        const char * const delimeter = (pos++ % qcolumns) == 0 ? "\n " : "";
        fprintf(f, ",%s %u", delimeter, *ptr);
    }
    fprintf(f, "\n};\n");

    return 0;
}

int ofsm_array_save_binary(const struct ofsm_array * const array, FILE * f, const char * const name)
{
    struct array_header header;
    memset(header.name, 0, 16);
    header.start_from = array->start_from;
    header.qflakes = array->qflakes;
    header.len = array->len;
    strncpy(header.name, name, 16);

    const size_t header_sz = sizeof(struct array_header);
    const size_t written1 = fwrite(&header, 1, header_sz, f);
    if (written1 != header_sz) {
        return 1;
    }

    const size_t sz = header.len * sizeof(uint32_t);
    const size_t written2 = fwrite(array->array, 1, sz, f);
    if (written2 != sz) {
        return 1;
    }

    return 0;
}
