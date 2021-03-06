#ifndef YOO__FSM__H__
#define YOO__FSM__H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <yoo-stdlib.h>
#include <yoo-combinatoric.h>



#define input_t         uint8_t
#define state_t         uint32_t
#define pack_value_t    uint64_t

#define INVALID_INPUT ((input_t)(~0))
#define INVALID_STATE ((state_t)(~0))
#define INVALID_PACK_VALUE ((pack_value_t)(~0))
#define INVALID_HASH (~0ull)

#define OFSM_STACK_SZ   16

#define PACK_FLAG__SKIP_RENUMERING     1

#define OBF__OWN_MEMPOOL    1
#define OBF__AUTO_VERIFY    2



struct ofsm_array
{
    uint32_t start_from;
    uint32_t qflakes;
    uint64_t len;
    uint32_t * array;
};

struct array_header
{
    char name[16];
    uint32_t start_from;
    uint32_t qflakes;
    uint64_t len;
};



typedef pack_value_t pack_func(void * user_data, unsigned int n, const input_t * path);
typedef uint64_t hash_func(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path);



struct ofsm_builder
{
    struct mempool * restrict mempool;
    int flags;
    FILE * logstream;
    FILE * errstream;
    void * stack[OFSM_STACK_SZ];
    unsigned int stack_len;
    void * user_data;
    struct choose_table choose;
};



struct ofsm_builder * create_ofsm_builder(struct mempool * restrict const arg_mempool, FILE * const errstream);
void free_ofsm_builder(struct ofsm_builder * restrict const me);
int ofsm_builder_make_array(const struct ofsm_builder * const me, const unsigned int delta_last, struct ofsm_array * restrict const out);

int ofsm_builder_push_pow(struct ofsm_builder * restrict const me, const input_t qinputs, const unsigned int m);
int ofsm_builder_push_comb(struct ofsm_builder * restrict const me, const input_t qinputs, const unsigned int m);
int ofsm_builder_product(struct ofsm_builder * restrict const me);
int ofsm_builder_pack(struct ofsm_builder * restrict const me, pack_func f, const unsigned int flags);
int ofsm_builder_optimize(struct ofsm_builder * restrict const me, const unsigned int nflake, unsigned int qflakes, hash_func f);
int ofsm_builder_verify(const struct ofsm_builder * const me);



const void * ofsm_builder_get_ofsm(const struct ofsm_builder * const me);
const input_t * ofsm_get_path(const void * ofsm, unsigned int nflake, state_t output);
state_t ofsm_execute(const void * const ofsm, const unsigned int n, const input_t * const inputs);
int ofsm_get_array(const void * const ofsm, const unsigned int delta_last, struct ofsm_array * restrict const out);



int ofsm_array_print(const struct ofsm_array * const array, FILE * const f, const char * const name, const unsigned int arg_qcolumns);
int ofsm_array_save_binary(const struct ofsm_array * const array, FILE * f, const char * const name);



#endif
