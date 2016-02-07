#ifndef YOO__FSM__H__
#define YOO__FSM__H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>



#define input_t         uint8_t
#define state_t         uint32_t
#define pack_value_t    uint64_t

#define PACK_FLAG__SKIP_RENUMERING     1

#define INVALID_INPUT (~(input_t)0)
#define INVALID_STATE (~(state_t)0)
#define INVALID_PACK_VALUE (~(pack_value_t)0)



struct ofsm_array
{
    unsigned int start_from;
    unsigned int qflakes;
    uint64_t len;
    unsigned int * array;
};

typedef void build_script_func(void * script);
typedef int check_ofsm_func(const void * ofsm);
typedef pack_value_t pack_func(unsigned int n, const input_t * path);
typedef uint64_t hash_func(unsigned int n, const state_t * path);

int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check);

void script_step_pow(void * restrict script, input_t qinputs, unsigned int m);
void script_step_comb(void * restrict script, input_t qinputs, unsigned int m);
void script_step_pack(void * restrict script, pack_func f, unsigned int flags);
void script_step_optimize(void * restrict script, unsigned int nflake, hash_func f);

int ofsm_execute(const void * ofsm, unsigned int n, const int * inputs);
int ofsm_get_array(const void * ofsm, unsigned int delta_last, struct ofsm_array * restrict out);
int ofsm_print_array(FILE * f, const char * name, const struct ofsm_array * array, unsigned int qcolumns);

#endif
