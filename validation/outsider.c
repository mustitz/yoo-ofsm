#include "yoo-ofsm.h"

#include <stdlib.h>
#include <string.h>



int optimize_with_hash_path_test(void);
int optimize_with_invalid_hash_test(void);
int optimize_with_zero_hash_test(void);
int optimize_with_rnd_hash_test(void);
int optimize_test(void);
int pack_without_renum_test(void);
int pack_test(void);
int pow_41_comb_52_test(void);
int pow_41_pow_51_test(void);
int comb_55_test(void);
int comb_42_test(void);
int pow_42_test(void);
int pow_41_test(void);
int empty_builder_test(void);
int empty_test(void);



typedef int (* test_f)(void);

struct test_item
{
    const char * name;
    test_f func;
};

#define TEST_ITEM(name) { #name, &name##_test }
struct test_item tests[] = {
    TEST_ITEM(empty),
    TEST_ITEM(optimize_with_hash_path),
    TEST_ITEM(optimize_with_invalid_hash),
    TEST_ITEM(optimize_with_zero_hash),
    TEST_ITEM(optimize_with_rnd_hash),
    TEST_ITEM(optimize),
    TEST_ITEM(pack_without_renum),
    TEST_ITEM(pack),
    TEST_ITEM(pow_41_comb_52),
    TEST_ITEM(pow_41_pow_51),
    TEST_ITEM(comb_55),
    TEST_ITEM(comb_42),
    TEST_ITEM(pow_42),
    TEST_ITEM(pow_41),
    TEST_ITEM(empty_builder),
    { NULL, NULL }
};
#undef TEST_ITEM



struct test_item * current_test;

static void print_tests(void)
{
    const struct test_item * current = tests;
    for (; current->name != NULL; ++current) {
        printf("%s\n", current->name);
    }
}

static void run_test_item(const struct test_item * const item)
{
    printf("Run test “%s”.\n", item->name);
    const int status = (*item->func)();
    if (status != 0) {
        exit(status);
    }
}

static void run_test(const char * const name)
{
    for (current_test = tests; current_test->name != NULL; ++current_test) {
        if (strcmp(name, current_test->name) == 0) {
            return run_test_item(current_test);
        }
    }

    fprintf(stderr, "Test “%s” is not found.\n", name);
    exit(1);
}

int empty_test(void)
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



static void print_path(const char * const prefix, const input_t * const path, const size_t len)
{
    fprintf(stderr, "%s", prefix);
    for (size_t i=0; i<len; ++i) {
        fprintf(stderr, " %d", path[i]);
    }
    fprintf(stderr, ".\n");
}



static unsigned int run_array(const struct ofsm_array * const array, const input_t * const input)
{
    const unsigned int n = array->qflakes;
    unsigned int current = array->start_from;
    for (unsigned int i=0; i<n; ++i) {
        current = array->array[current + input[i]];
    }
    return current;
}



static pack_value_t mod7(void * const user_data, const unsigned int n, const input_t * const path)
{
    return ((3*path[0] + path[1] + path[2]) % 7) + 11;
}

static uint64_t rnd_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    uint64_t result = 0;
    for (unsigned int i = 0; i < 1; ++i) {
        result += jumps[i];
    }
    return result % 2;
}

static uint64_t zero_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return 0;
}

static uint64_t invalid_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return INVALID_HASH;
}



int empty_builder_test(void)
{
    struct mempool * restrict const mempool = create_mempool(2000);
    struct ofsm_builder * restrict const ofsm_builder = create_ofsm_builder(mempool, stderr);
    free_ofsm_builder(ofsm_builder);
    free_mempool(mempool);
    return 0;
}



int pow_41_test(void)
{
    static const unsigned int NFLAKE = 1;
    static const state_t QOUTS = 4;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 1;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0]) {
        const state_t value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int pow_42_test(void)
{
    static const unsigned int NFLAKE = 2;
    static const state_t QOUTS = 16;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 1;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 2) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        const state_t value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int comb_42_test(void)
{
    static const unsigned int NFLAKE = 2;
    static const state_t QOUTS = 6;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 2;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_comb(me, 4, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 4, 2) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<4; ++c[1]) {
        const state_t value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (c[0] == c[1]) {
            if (value != 0) {
                fprintf(stderr, "Invalid value (%u) after run_array: should be 0 via invalid path.", value);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            if (state != INVALID_STATE) {
                fprintf(stderr, "Invalid state (%u) after script_execute: expected INVALID_STATE (%u).\n", state, INVALID_STATE);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            continue;
        }

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int comb_55_test(void)
{
    static const unsigned int NFLAKE = 5;
    static const unsigned int QOUTS = 1;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 120;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_comb(me, 5, 5);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 5, 5) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<5; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2])
    for (c[3]=0; c[3]<5; ++c[3])
    for (c[4]=0; c[4]<5; ++c[4]) {
        const unsigned int value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        const unsigned int mask = 0
            | (1 << c[0])
            | (1 << c[1])
            | (1 << c[2])
            | (1 << c[3])
            | (1 << c[4])
        ;

        if (mask != 0x1F) {
            if (value != 0) {
                fprintf(stderr, "Invalid value (%u) after run_array: should be 0 via invalid path.", value);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            if (state != INVALID_STATE) {
                fprintf(stderr, "Invalid state (%u) after script_execute: expected INVALID_STATE (%u).\n", state, INVALID_STATE);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            continue;
        }

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int pow_41_pow_51_test(void)
{
    static const unsigned int NFLAKE = 2;
    static const state_t QOUTS = 20;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 1;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_push_pow(me, 5, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 5, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_product(me);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_product(me) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1]) {
        const state_t value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int pow_41_comb_52_test(void)
{
    static const unsigned int NFLAKE = 3;
    static const state_t QOUTS = 40;
    static const unsigned int DELTA = 1;
    static const int EXPECTED_STAT = 2;

    int status;
    int stat[QOUTS];
    memset(stat, 0, sizeof(stat));

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_push_comb(me, 5, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 5, 2) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_product(me);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_product(me) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        state_t value = run_array(&array, c);
        state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (c[1] == c[2]) {
            if (value != 0) {
                fprintf(stderr, "Invalid value (%u) after run_array: should be 0 via invalid path.", value);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            if (state != INVALID_STATE) {
                fprintf(stderr, "Invalid state (%u) after script_execute: expected INVALID_STATE (%u).\n", state, INVALID_STATE);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            continue;
        }

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range 1 - %u.\n", value, QOUTS);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        ++stat[state];
    }

    for (int i=0; i<QOUTS; ++i) {
        if (stat[i] != EXPECTED_STAT) {
            fprintf(stderr, "Invalid stat[%d] = %d, excpected value is %d.\n", i, stat[i], EXPECTED_STAT);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int pack_test(void)
{
    static const unsigned int NFLAKE = 3;
    static const state_t QOUTS = 7;
    static const unsigned int DELTA = 1;

    int status;

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_push_comb(me, 5, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 5, 2) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_product(me);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_product(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_pack(me, mod7, 0);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_pack(me) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        const unsigned int value = run_array(&array, c);

        const state_t state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[1] == c[2]) {
            if (value != 0) {
                fprintf(stderr, "Invalid value (%u) after run_array: should be 0 via invalid path.", value);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            if (state != INVALID_STATE) {
                fprintf(stderr, "Invalid state (%u) after script_execute: expected INVALID_STATE (%u).\n", state, INVALID_STATE);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            continue;
        }

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range %u - %u.\n", value, DELTA, DELTA + QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        const pack_value_t expected = (3*c[0] + c[1] + c[2]) % 7;
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%u) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int pack_without_renum_test(void)
{
    static const unsigned int NFLAKE = 3;
    static const unsigned int DELTA = 0;

    int status;

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_push_comb(me, 5, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 5, 2) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_product(me);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_product(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_pack(me, mod7, PACK_FLAG__SKIP_RENUMERING);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_pack(me) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        const unsigned int value = run_array(&array, c);

        const state_t state = ofsm_execute(ofsm, NFLAKE, c);
        if (c[1] == c[2]) {
            if (value != 0) {
                fprintf(stderr, "Invalid value (%u) after run_array: should be 0 via invalid path.", value);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            if (state != INVALID_STATE) {
                fprintf(stderr, "Invalid state (%u) after script_execute: expected INVALID_STATE (%u).\n", state, INVALID_STATE);
                print_path("input =", c, NFLAKE);
                return 1;
            }

            continue;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        const pack_value_t expected = mod7(NULL, NFLAKE, c);
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%u) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }
    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}



int optimize_test_with_hash(hash_func hash)
{
    static const unsigned int NFLAKE = 3;
    static const state_t QOUTS = 40;
    static const unsigned int DELTA = 0;

    int status;

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_pow(me, 4, 1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_push_comb(me, 5, 2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_comb(me, 5, 2) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_product(me);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_product(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_pack(me, mod7, 0);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_pack(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_optimize(me, 3, 1, hash);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_optimize(me, 3, 1, invalid_hash) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array;
    status = ofsm_builder_make_array(me, DELTA, &array);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    const void * const ofsm = ofsm_builder_get_ofsm(me);

    input_t c[NFLAKE];
    for (c[0]=0; c[0]<4; ++c[0])
    for (c[1]=0; c[1]<5; ++c[1])
    for (c[2]=0; c[2]<5; ++c[2]) {
        const unsigned int value = run_array(&array, c);
        const state_t state = ofsm_execute(ofsm, NFLAKE, c);

        if (c[1] == c[2]) {
            continue;
        }

        if (value < DELTA || value >= QOUTS + DELTA) {
            fprintf(stderr, "Invalid value (%u) after run_array, out of range %u - %u.\n", value, DELTA, DELTA + QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state >= QOUTS) {
            fprintf(stderr, "Invalid state (%u) after script_execute: out of range 0 - %u.\n", state, QOUTS - 1);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        if (state != value - DELTA) {
            fprintf(stderr, "state & value-DELTA mismatch: state = %u, value = %u, DELTA = %u.\n", state, value, DELTA);
            print_path("input =", c, NFLAKE);
            return 1;
        }

        const input_t * const path = ofsm_get_path(ofsm, NFLAKE, state);
        if (path == NULL) {
            fprintf(stderr, "ofsm_get_path(ofsm, %u) failed with NULL as result.\n", state);
            return 1;
        }

        const state_t state2 = ofsm_execute(ofsm, NFLAKE, path);
        if (state != state2) {
            fprintf(stderr, "Invalid path in OFSM, state = %u, state2 = %u.\n", state, state2);
            print_path("input =", c, NFLAKE);
            print_path("path =", path, NFLAKE);
            return 1;
        }

        const size_t expected = (3*c[0] + c[1] + c[2]) % 7;
        if (expected != state) {
            fprintf(stderr, "Unexpected state (%u) after script_execute: expected %lu.\n", state, expected);
            print_path("input =", c, NFLAKE);
            return 1;
        }

    }

    free(array.array);
    free_ofsm_builder(me);
    return 0;
}

int optimize_test(void)
{
    return optimize_test_with_hash(NULL);
}

int optimize_with_rnd_hash_test(void)
{
    return optimize_test_with_hash(rnd_hash);
}

int optimize_with_zero_hash_test(void)
{
    return optimize_test_with_hash(zero_hash);
}

int optimize_with_invalid_hash_test(void)
{
    return optimize_test_with_hash(invalid_hash);
}

pack_value_t sum_with_bonus(void * const user_data, const unsigned int n, const input_t * const path)
{
    unsigned int sum = 0;
    unsigned int mask = 0;
    for (int i=0; i<n; ++i) {
        const unsigned int value = path[i] / 2;
        const unsigned int type = path[i] % 2;
        mask |= 1 << type;
        sum += value;
    }

    if (mask == 1 || mask == 2) {
        sum *= 3;
    }

    return sum;
}

static uint64_t forget_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    unsigned int sum = 0;
    unsigned int mask = 0;
    for (int i=0; i<path_len; ++i) {
        const unsigned int value = path[i] / 2;
        const unsigned int type = path[i] % 2;
        mask |= 1 << type;
        sum += value;
    }

    return sum | (mask << 24);
}

int optimize_with_hash_path_test(void)
{
    int status;

    struct ofsm_builder * restrict const me = create_ofsm_builder(NULL, stderr);
    if (me == NULL) {
        fprintf(stderr, "create_ofsm_builder failed with NULL as a error value.");
        return 1;
    }

    me->flags |= OBF__AUTO_VERIFY;

    status = ofsm_builder_push_comb(me, 10, 5);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_push_pow(me, 4, 1) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_pack(me, sum_with_bonus, PACK_FLAG__SKIP_RENUMERING);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_pack(me) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array1;
    status = ofsm_builder_make_array(me, 0, &array1);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_optimize(me, 5, 1, forget_hash);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_optimize(me, 5, 1, invalid_hash) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array2;
    status = ofsm_builder_make_array(me, 0, &array2);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    status = ofsm_builder_optimize(me, 5, 0, NULL);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_optimize(me, 5, 0, NULL) failed with %d as error code.\n", status);
        return 1;
    }

    struct ofsm_array array3;
    status = ofsm_builder_make_array(me, 0, &array3);
    if (status != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.\n", status);
        return 1;
    }

    free(array1.array);
    free(array2.array);
    free(array3.array);
    free_ofsm_builder(me);
    return 0;
}
