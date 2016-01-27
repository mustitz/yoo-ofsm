#ifndef YOO__FSM__H__
#define YOO__FSM__H__

typedef void build_script_func();

int execute(int argc, char * argv[], build_script_func f);
#endif
