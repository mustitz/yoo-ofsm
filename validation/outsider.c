#include "ofsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int empty_test();
int append_power_41_test();
int append_power_42_test();
int append_power_41_51_test();
int append_combinatoric_42_test();
int append_combinatoric_55_test();
int append_power_41_combinatoric_52_test();
int append_pack_test();



typedef int (* test_f)();

struct test_item
{
    const char * name;
    test_f func;
};

#define TEST_ITEM(name) { #name, &name##_test }
struct test_item tests[] = {
    TEST_ITEM(empty),
    TEST_ITEM(append_pack),
    TEST_ITEM(append_power_41_combinatoric_52),
    TEST_ITEM(append_combinatoric_55),
    TEST_ITEM(append_combinatoric_42),
    TEST_ITEM(append_power_41_51),
    TEST_ITEM(append_power_42),
    TEST_ITEM(append_power_41),
    { NULL, NULL }
};
#undef TEST_ITEM



struct test_item * current_test;

void print_tests()
{
    struct test_item * current = tests;
    for (; current->name != NULL; ++current) {
        printf("%s\n", current->name);
    }
}

void run_test_item(const struct test_item * item)
{
    printf("Run test “%s”.\n", item->name);
    int code = (*item->func)();
    if (code) {
        exit(code);
    }
}

void run_test(const char * name)
{
    for (current_test = tests; current_test->name != NULL; ++current_test) {
        if (strcmp(name, current_test->name) == 0) {
            return run_test_item(current_test);
        }
    }

    fprintf(stderr, "Test “%s” is not found.\n", name);
    exit(1);
}

int empty_test()
{
    return 0;
}

int main(int argc, char * argv[])
{
    if (argc <= 1) {
        print_tests();
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        run_test(argv[i]);
    }

    return 0;
}



static void print_int_array(const char * prefix, const int * data, size_t len)
{
    fprintf(stderr, "%s", prefix);
    for (size_t i=0; i<len; ++i) {
        fprintf(stderr, " %d", data[i]);
    }
    fprintf(stderr, ".\n");
}



void build_append_power_41(void * script)
{
    script_append_power(script, 4, 1);
}

int check_append_power_41(const void * ofsm)
{
    static int stat[4];
    memset(stat, 0, sizeof(stat));

    int c[1];
    for (c[0]=0; c[0]<4; ++c[0]) {
        int state = ofsm_execute(ofsm, 1, c);
        if (state < 0 || state >= 4) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 3.\n", state);
            print_int_array("input =", c, 1);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<4; ++i) {
        if (stat[i] != 1) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is 1.\n", i, stat[i]);
            return 1;
        }
    }

    return 0;
}

int append_power_41_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_power_41, check_append_power_41);
}



void build_append_power_42(void * script)
{
    script_append_power(script, 4, 2);
}

int check_append_power_42(const void * ofsm)
{
    static int stat[16];
    memset(stat, 0, sizeof(stat));

    int c[2];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        int state = ofsm_execute(ofsm, 2, c);
        if (state < 0 || state >= 16) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 15.\n", state);
            print_int_array("input =", c, 2);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<16; ++i) {
        if (stat[i] != 1) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is 1.\n", i, stat[i]);
            return 1;
        }
    }

    return 0;
}

int append_power_42_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_power_42, check_append_power_42);
}




void build_append_power_41_51(void * script)
{
    script_append_power(script, 4, 1);
    script_append_power(script, 5, 1);
}

int check_append_power_41_51(const void * ofsm)
{
    static int stat[20];
    memset(stat, 0, sizeof(stat));

    int c[2];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1]) {
        int state = ofsm_execute(ofsm, 2, c);
        if (state < 0 || state >= 20) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 19.\n", state);
            print_int_array("input =", c, 2);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<20; ++i) {
        if (stat[i] != 1) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is 1.\n", i, stat[i]);
            return 1;
        }
    }

    return 0;
}

int append_power_41_51_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_power_41_51, check_append_power_41_51);
}



void build_append_combinatoric_42(void * script)
{
    script_append_combinatoric(script, 4, 2);
}

int check_append_combinatoric_42(const void * ofsm)
{
    static int stat[6];
    memset(stat, 0, sizeof(stat));

    int c[2];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        int state = ofsm_execute(ofsm, 2, c);
        if (c[0] == c[1]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_int_array("input =", c, 2);
            return 1;
        }

        if (state < 0 || state >= 6) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 5.\n", state);
            print_int_array("input =", c, 2);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<5; ++i) {
        if (stat[i] != 2) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is 2.\n", i, stat[i]);
            return 1;
        }
    }

    return 0;
}

int append_combinatoric_42_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_combinatoric_42, check_append_combinatoric_42);
}



void build_append_combinatoric_55(void * script)
{
    script_append_combinatoric(script, 5, 5);
}

int check_append_combinatoric_55(const void * ofsm)
{
    static int stat = 0;

    int c[5];
    for (c[0]=0; c[0]<5; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2])
    for (c[3]=0; c[3]<5; ++c[3])
    for (c[4]=0; c[4]<5; ++c[4]) {
        int state = ofsm_execute(ofsm, 5, c);

        unsigned int mask = 0
            | (1 << c[0])
            | (1 << c[1])
            | (1 << c[2])
            | (1 << c[3])
            | (1 << c[4])
        ;

        if (mask != 0x1F) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_int_array("input =", c, 5);
            return 1;
        }

        if (state != 0) {
            fprintf(stderr, "Invalid state (%d) after script_execute: expected 0.\n", state);
            print_int_array("input =", c, 5);
            return 1;
        }

        ++stat;
    }

    if (stat != 120) {
        fprintf(stderr, "Invalid stat = %d, excpected value is .\n", stat);
        return 1;
    }

    return 0;
}

int append_combinatoric_55_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_combinatoric_55, check_append_combinatoric_55);
}



void build_append_power_41_combinatoric_52(void * script)
{
    script_append_power(script, 4, 1);
    script_append_combinatoric(script, 5, 2);
}

int check_append_power_41_combinatoric_52(const void * ofsm)
{
    static int stat[40];
    memset(stat, 0, sizeof(stat));

    int c[3];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        int state = ofsm_execute(ofsm, 3, c);
        if (c[1] == c[2]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_int_array("input =", c, 3);
            return 1;
        }

        if (state < 0 || state >= 40) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 39.\n", state);
            print_int_array("input =", c, 3);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<40; ++i) {
        if (stat[i] != 2) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is 2.\n", i, stat[i]);
            return 1;
        }
    }

    return 0;
}

int append_power_41_combinatoric_52_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_power_41_combinatoric_52, check_append_power_41_combinatoric_52);
}



pack_value_t mod7(unsigned int n, const input_t * path)
{
    return (3*path[0] + path[1] + path[2]) % 7;
}

void build_append_pack(void * script)
{
    script_append_power(script, 4, 1);
    script_append_combinatoric(script, 5, 2);
    script_append_pack(script, mod7);
}

int check_append_pack(const void * ofsm)
{
    return 0;
}

int append_pack_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_append_pack, check_append_pack);
}
