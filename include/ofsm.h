#ifndef YOO__FSM__H__
#define YOO__FSM__H__

#include <stddef.h>
#include <stdint.h>



#define input_t         uint8_t
#define state_t         uint32_t
#define pack_value_t    uint64_t

#define INVALID_STATE (~(state_t)0)
#define INVALID_PACK_VALUE (~(pack_value_t)0)



typedef void build_script_func(void * script);
typedef int check_ofsm_func(const void * ofsm);
typedef pack_value_t pack_func(unsigned int n, const input_t * path);

int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check);

void script_append_power(void * restrict script, unsigned int n, unsigned int m);
void script_append_combinatoric(void * restrict script, unsigned int n, unsigned int m);
void script_append_pack(void * restrict script, pack_func f);
void script_optimize(void * restrict script, unsigned int nflake);

int ofsm_execute(const void * ofsm, unsigned int n, const int * inputs);

#endif
