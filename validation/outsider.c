#include "ofsm.h"

#include <stdlib.h>
#include <string.h>



int empty_test();
int pow_41_test();
int pow_42_test();
int pow_41_51_test();
int comb_42_test();
int comb_55_test();
int pow_41_comb_52_test();
int pack_test();
int pack_without_renum_test();
int optimize_test();



typedef int (* test_f)();

struct test_item
{
    const char * name;
    test_f func;
};

#define TEST_ITEM(name) { #name, &name##_test }
struct test_item tests[] = {
    TEST_ITEM(empty),
    TEST_ITEM(optimize),
    TEST_ITEM(pack_without_renum),
    TEST_ITEM(pack),
    TEST_ITEM(pow_41_comb_52),
    TEST_ITEM(comb_55),
    TEST_ITEM(comb_42),
    TEST_ITEM(pow_41_51),
    TEST_ITEM(pow_42),
    TEST_ITEM(pow_41),
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



static void print_path(const char * prefix, const input_t * path, size_t len)
{
    fprintf(stderr, "%s", prefix);
    for (size_t i=0; i<len; ++i) {
        fprintf(stderr, " %d", path[i]);
    }
    fprintf(stderr, ".\n");
}



static unsigned int run_array(const struct ofsm_array * array, const input_t * input)
{
    unsigned int n = array->qflakes;
    unsigned int current = array->start_from;
    for (unsigned int i=0; i<n; ++i) {
        current = array->array[current + input[i]];
    }
    return current;
}


void build_pow_41(void * script)
{
    script_step_pow(script, 4, 1);
}

int check_pow_41(const void * ofsm)
{
    static const unsigned int NFLAKE = 1;

    static int stat[4];
    memset(stat, 0, sizeof(stat));

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0]) {
        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (state < 0 || state >= 4) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 3.\n", state);
            print_path("input =", c, NFLAKE);
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

int pow_41_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pow_41, check_pow_41);
}



void build_pow_42(void * script)
{
    script_step_pow(script, 4, 2);
}

int check_pow_42(const void * ofsm)
{
    static const unsigned int NFLAKE = 2;

    static int stat[16];
    memset(stat, 0, sizeof(stat));

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 0, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (state < 0 || state >= 16) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 15.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value) {
            fprintf(stderr, "state & value mismatch: state = %d, value = %d.\n", state, value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        value = ofsm_execute(ofsm, NFLAKE, path);
        if (state != value) {
            fprintf(stderr, "Invalid path in OFSM, state = %d, value = %d.\n", state, value);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
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

    free(array.array);
    return 0;
}

int pow_42_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pow_42, check_pow_42);
}




void build_pow_41_51(void * script)
{
    script_step_pow(script, 4, 1);
    script_step_pow(script, 5, 1);
}

int check_pow_41_51(const void * ofsm)
{
    static const unsigned int NFLAKE = 2;

    static int stat[20];
    memset(stat, 0, sizeof(stat));

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 0, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[2];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (state < 0 || state >= 20) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 19.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value) {
            fprintf(stderr, "state & value mismatch: state = %d, value = %d.\n", state, value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        value = ofsm_execute(ofsm, NFLAKE, path);
        if (state != value) {
            fprintf(stderr, "Invalid path in OFSM, state = %d, value = %d.\n", state, value);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
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

    free(array.array);
    return 0;
}

int pow_41_51_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pow_41_51, check_pow_41_51);
}



void build_comb_42(void * script)
{
    script_step_comb(script, 4, 2);
}

int check_comb_42(const void * ofsm)
{
    static const unsigned int NFLAKE = 2;

    static int stat[6];
    memset(stat, 0, sizeof(stat));

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.", errcode);
        return 1;
    }

    input_t c[2];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[0] == c[1]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state < 0 || state >= 6) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 5.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - 1) {
            fprintf(stderr, "state & value mismatch: state = %d, value-1 = %d.\n", state, value-1);
            print_path("input =", c, NFLAKE);
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

    free(array.array);
    return 0;
}

int comb_42_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_comb_42, check_comb_42);
}



void build_comb_55(void * script)
{
    script_step_comb(script, 5, 5);
}

int check_comb_55(const void * ofsm)
{
    static const unsigned int NFLAKE = 5;

    static int stat = 0;

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[5];
    for (c[0]=0; c[0]<5; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2])
    for (c[3]=0; c[3]<5; ++c[3])
    for (c[4]=0; c[4]<5; ++c[4]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);

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
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != 0) {
            fprintf(stderr, "Invalid state (%d) after script_execute: expected 0.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - 1) {
            fprintf(stderr, "state & value mismatch: state = %d, value-1 = %d.\n", state, value-1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        ++stat;
    }

    if (stat != 120) {
        fprintf(stderr, "Invalid stat = %d, excpected value is 120.\n", stat);
        return 1;
    }

    free(array.array);
    return 0;
}

int comb_55_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_comb_55, check_comb_55);
}



void build_pow_41_comb_52(void * script)
{
    script_step_pow(script, 4, 1);
    script_step_comb(script, 5, 2);
}

int check_pow_41_comb_52(const void * ofsm)
{
    static const unsigned int NFLAKE = 3;

    static int stat[40];
    memset(stat, 0, sizeof(stat));

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[3];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[1] == c[2]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state < 0 || state >= 40) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 39.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - 1) {
            fprintf(stderr, "state & value mismatch: state = %d, value-1 = %d.\n", state, value-1);
            print_path("input =", c, NFLAKE);
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

    free(array.array);
    return 0;
}

int pow_41_comb_52_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pow_41_comb_52, check_pow_41_comb_52);
}



pack_value_t mod7(unsigned int n, const input_t * path)
{
    return ((3*path[0] + path[1] + path[2]) % 7) + 11;
}

void build_pack(void * script)
{
    script_step_pow(script, 4, 1);
    script_step_comb(script, 5, 2);
    script_step_pack(script, mod7, 0);
}

int check_pack(const void * ofsm)
{
    static const unsigned int NFLAKE = 3;

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[3];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[1] == c[2]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state < 0 || state >= 7) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 6.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        pack_value_t expected = (3*c[0] + c[1] + c[2]) % 7;
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%d) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - 1) {
            fprintf(stderr, "state & value mismatch: state = %d, value-1 = %d.\n", state, value-1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

    }

    free(array.array);
    return 0;
}

int pack_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pack, check_pack);
}



void build_pack_without_renum(void * script)
{
    script_step_pow(script, 4, 1);
    script_step_comb(script, 5, 2);
    script_step_pack(script, mod7, PACK_FLAG__SKIP_RENUMERING);
}

int check_pack_without_renum(const void * ofsm)
{
    static const unsigned int NFLAKE = 3;

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 0, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[3];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[1] == c[2]) {
            if (state == -1) continue;
            fprintf(stderr, "Invalid state (%d) after script_execute: expected -1.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state < 11 || state >= 18) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 11 - 17.\n", state);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        pack_value_t expected = mod7(NFLAKE, c);
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%d) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value) {
            fprintf(stderr, "state & value mismatch: state = %d, value = %d.\n", state, value);
            print_path("input =", c, 3);
            return 1;
        }

    }

    free(array.array);
    return 0;
}

int pack_without_renum_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_pack_without_renum, check_pack_without_renum);
}



void build_optimize(void * script)
{
    script_step_pow(script, 4, 1);
    script_step_comb(script, 5, 2);
    script_step_pack(script, mod7, 0);
    script_step_optimize(script, 3, NULL);
}

int check_optimize(const void * ofsm)
{
    static const unsigned int NFLAKE = 3;

    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    input_t c[3];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {

        if (c[1] == c[2]) {
            // Optimized
            continue;
        }

        int value = (unsigned int)run_array(&array, c);
        if (value < 0) {
            fprintf(stderr, "Invalid value (%d) after run_array.\n", value);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        int state = ofsm_execute(ofsm, NFLAKE, c);
        if (state < 0 || state >= 7) {
            fprintf(stderr, "Invalid state (%d) after script_execute: out of range 0 - 6.\n", state);
            print_path("input =", c, 3);
            return 1;
        }

        size_t expected = (3*c[0] + c[1] + c[2]) % 7;
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%d) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - 1) {
            fprintf(stderr, "state & value mismatch: state = %d, value-1 = %d.\n", state, value-1);
            return 1;
        }
    }

    free(array.array);
    return 0;
}

int optimize_test()
{
    char * argv[2] = { "outsider", "-v" };
    return execute(1, argv, build_optimize, check_optimize);
}
