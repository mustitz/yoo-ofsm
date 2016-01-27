#include <fsm.h>

struct script
{
};

struct script * create_script()
{
    return NULL;
}

void free_script(struct script * restrict script)
{
}

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

    struct script * restrict script;
    script = create_script();

    free_script(script);
    return 0;
}
