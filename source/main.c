#include <ofsm.h>

int64_t calc_rank(int64_t value)
{
    return value;
}

void build_script(void * script)
{
    script_append_combinatoric(script, 52, 5);
    script_pack(script, calc_rank);
    script_optimize(script, 5);
    script_optimize(script, 4);
    script_optimize(script, 3);
    script_optimize(script, 2);
    script_optimize(script, 1);
}

int main(int argc, char * argv[])
{
    return execute(argc, argv, build_script);
}
