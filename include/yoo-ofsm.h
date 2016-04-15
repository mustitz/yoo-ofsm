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

#define PACK_FLAG__SKIP_RENUMERING     1

#define INVALID_INPUT (~(input_t)0)
#define INVALID_STATE (~(state_t)0)
#define INVALID_PACK_VALUE (~(pack_value_t)0)
#define INVALID_HASH (~0ull)

#define OFSM_STACK_SZ   16


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


typedef void build_script_func(void * script);
typedef int check_ofsm_func(const void * ofsm);
typedef pack_value_t pack_func(unsigned int n, const input_t * path);
typedef uint64_t hash_func(unsigned int n, const state_t * jumps);




int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check);

void script_step_pow(void * restrict script, input_t qinputs, unsigned int m);
void script_step_comb(void * restrict script, input_t qinputs, unsigned int m);
void script_step_pack(void * restrict script, pack_func f, unsigned int flags);
void script_step_optimize(void * restrict script, unsigned int nflake, hash_func f);

state_t ofsm_execute(const void * ofsm, unsigned int n, const input_t * inputs);
int ofsm_get_array(const void * ofsm, unsigned int delta_last, struct ofsm_array * restrict out);
const input_t * ofsm_get_path(const void * ofsm, unsigned int nflake, state_t output);

int ofsm_print_array(FILE * f, const char * name, const struct ofsm_array * array, unsigned int qcolumns);
int ofsm_save_binary_array(FILE * f, const char * name, const struct ofsm_array * array);
void save_binary(const char * file_name, const char * name, const struct ofsm_array * array);



struct ofsm_builder
{
    struct mempool * restrict mempool;
    int own_mempool;
    FILE * logstream;
    FILE * errstream;
    void * ofsm_stack[OFSM_STACK_SZ];
    size_t ofsm_stack_first;
    size_t ofsm_stack_last;
    struct choose_table choose;
};

struct ofsm_builder * create_ofsm_builder(struct mempool * restrict mempool, FILE * errstream);
void free_ofsm_builder(struct ofsm_builder * restrict me);
int ofsm_builder_make_array(const struct ofsm_builder * me, unsigned int delta_last, struct ofsm_array * restrict out);

int ofsm_builder_push_comb(struct ofsm_builder * restrict me, input_t qinputs, unsigned int m);
int ofsm_builder_pack(struct ofsm_builder * restrict me, pack_func f, unsigned int flags);
int ofsm_builder_optimize(struct ofsm_builder * restrict me, unsigned int first_nflake, unsigned int last_nflake, hash_func f);

#endif
