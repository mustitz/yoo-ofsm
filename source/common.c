#include <fsm.h>

static int parse_command_line(int argc, char * argv[])
{
    return 0;
}

int execute(int argc, char * argv[], build_script_func f)
{
    int errcode = parse_command_line(argc, argv);
    if (errcode != 0) {
        return errcode;
    }

    return 0;
}
