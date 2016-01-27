#include <fsm.h>

#include <stdarg.h>
#include <stdio.h>

#define errheader errmsg("Error in function “%s”, file %s:%d.", __FUNCTION__, __FILE__, __LINE__)

static void verrmsg(const char * fmt, va_list args)
{
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static void errmsg(const char * fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void errmsg(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verrmsg(fmt, args);
    va_end(args);
}

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
    if (f == NULL) {
        errheader;
        errmsg("Invalid argument “f” = NULL (build script function). Not NULL value is expected.");
        return 1;
    }

    int errcode = parse_command_line(argc, argv);
    if (errcode != 0) {
        return errcode;
    }

    struct script * restrict script;
    script = create_script();

    f(script);

    free_script(script);
    return 0;
}
