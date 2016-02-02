#ifndef YOO__FSM__H__
#define YOO__FSM__H__

#include <stddef.h>
#include <stdint.h>

#define input_t unsigned char
#define state_t uint32_t

typedef void build_script_func(void * script);
typedef int check_ofsm_func(const void * ofsm);
typedef int64_t pack_func(unsigned int n, const input_t * path);

int execute(int argc, char * argv[], build_script_func build, check_ofsm_func check);

void script_append_power(void * restrict script, unsigned int n, unsigned int m);
void script_append_combinatoric(void * restrict script, unsigned int n, unsigned int m);
void script_pack(void * restrict script, pack_func f);
void script_optimize(void * restrict script, int nlayer);

int ofsm_execute(const void * ofsm, unsigned int n, const int * inputs);

#endif
