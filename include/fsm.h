#ifndef YOO__FSM__H__
#define YOO__FSM__H__

#include <stddef.h>
#include <stdint.h>

typedef void build_script_func(void * script);
typedef int64_t pack_func(int64_t value);

int execute(int argc, char * argv[], build_script_func build);

void script_append_combinatoric(void * restrict script, int n, int m);
void script_pack(void * restrict script, pack_func pack);
void script_optimize(void * restrict script, int layer);

#endif
