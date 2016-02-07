#include "ofsm.h"

#include "poker.h"

#include <stdlib.h>
#include <string.h>

int is_prefix(const char * prefix, int * argc, char * argv[])
{
    if (*argc <= 1) return 0;
    if (strcmp(argv[1], prefix) != 0) return 0;

    --*argc;
    for (int i=1; i<*argc; ++i) {
        argv[i] = argv[i+1];
    }

    return 1;
}



#define PREFIX(hand_type)     ((uint64_t)HAND_TYPE__##hand_type << HAND_TYPE__FIRST_BIT)

uint64_t eval_slow_robust_rank5(const card_t * cards)
{
    int nominal_stat[13] = { 0 };
    uint64_t suite_mask = 0;
    uint64_t nominal_mask = 0;

    for (int i=0; i<5; ++i) {
        card_t card = cards[i];
        int nominal = NOMINAL(card);
        int suite = SUITE(card);
        ++nominal_stat[nominal];
        nominal_mask |= (1 << nominal);
        suite_mask |= (1 << suite);
    }

    suite_mask &= suite_mask - 1;
    int is_flush = suite_mask == 0;

    int is_straight = 0
        || nominal_mask == NOMINAL_MASK_5(A,K,Q,J,T)
        || nominal_mask == NOMINAL_MASK_5(K,Q,J,T,9)
        || nominal_mask == NOMINAL_MASK_5(Q,J,T,9,8)
        || nominal_mask == NOMINAL_MASK_5(J,T,9,8,7)
        || nominal_mask == NOMINAL_MASK_5(T,9,8,7,6)
        || nominal_mask == NOMINAL_MASK_5(9,8,7,6,5)
        || nominal_mask == NOMINAL_MASK_5(8,7,6,5,4)
        || nominal_mask == NOMINAL_MASK_5(7,6,5,4,3)
        || nominal_mask == NOMINAL_MASK_5(6,5,4,3,2)
        || nominal_mask == NOMINAL_MASK_5(5,4,3,2,A)
    ;

    if (is_straight) {
        if (nominal_mask == NOMINAL_MASK_5(5,4,3,2,A)) {
            nominal_mask = NOMINAL_MASK_4(5,4,3,2);
        }

        uint64_t prefix = is_flush ? PREFIX(STRAIGHT_FLUSH) : PREFIX(STRAIGHT);
        return prefix | nominal_mask;
    }

    if (is_flush) {
        return PREFIX(FLUSH) | nominal_mask;
    }

    uint64_t rank = 0;
    int stats[5] = { 0 };
    for (int n=NOMINAL_2; n<=NOMINAL_A; ++n) {
        if (nominal_stat[n] == 0) continue;
        ++stats[nominal_stat[n]];
        int shift = n + 13*nominal_stat[n] - 13;
        rank |= 1ull << shift;
    }

    uint64_t prefix = 0;

    if (0) ;
    else if (stats[4] == 1 && stats[3] == 0 && stats[2] == 0 && stats[1] == 1) prefix = PREFIX(FOUR_OF_A_KIND);
    else if (stats[4] == 0 && stats[3] == 1 && stats[2] == 1 && stats[1] == 0) prefix = PREFIX(FULL_HOUSE);
    else if (stats[4] == 0 && stats[3] == 1 && stats[2] == 0 && stats[1] == 2) prefix = PREFIX(THREE_OF_A_KIND);
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 2 && stats[1] == 1) prefix = PREFIX(TWO_PAIR);
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 1 && stats[1] == 3) prefix = PREFIX(ONE_PAIR);
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 0 && stats[1] == 5) prefix = PREFIX(HIGH_CARD);
    else {
        fprintf(stderr, "Assertion failed: invalid stats array: %d, %d, %d, %d, %d.\n", stats[0], stats[1], stats[2], stats[3], stats[4]);
        abort();
    }

    return rank | prefix;
}

pack_value_t calc_rank(unsigned int n, const input_t * path)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: omaha_simplify_5 requires n = 5, but %u as passed.\n", n);
    }
    card_t cards[5];
    for (size_t i=0; i<n; ++i) {
        cards[i] = path[i];
    }

    return eval_slow_robust_rank5(cards);
}

void build_holdem5_script(void * script)
{
    script_step_comb(script, 52, 5);
    script_step_pack(script, calc_rank, 0);
    script_step_optimize(script, 5, NULL);
    script_step_optimize(script, 4, NULL);
    script_step_optimize(script, 3, NULL);
    script_step_optimize(script, 2, NULL);
    script_step_optimize(script, 1, NULL);
}

int check_holdem5(const void * ofsm)
{
    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.", errcode);
        return 1;
    }

    ofsm_print_array(stdout, "fsm5_data", &array, 52);

    free(array.array);
    return 0;
}



pack_value_t omaha_simplify_5(unsigned int n, const input_t * path)
{
    static const uint64_t FLUSH              = 1ull << 60;
    static const uint64_t FOUR_OF_A_KIND     = 2ull << 60;
    static const uint64_t FULL_HOUSE         = 3ull << 60;
    static const uint64_t THREE_OF_A_KIND    = 4ull << 60;
    static const uint64_t TWO_PAIR           = 5ull << 60;
    static const uint64_t ONE_PAIR           = 6ull << 60;
    static const uint64_t HIGH_CARD          = 7ull << 60;

    if (n != 5) {
        fprintf(stderr, "Assertion failed: omaha_simplify_5 requires n = 5, but %u as passed.\n", n);
        abort();
    }

    int nominal_stat[13] = { 0 };
    int suite_stat[4] = { 0 };
    int stat[5] = { 0 };

    for (int i=0; i<5; ++i) {
        int nominal = path[i] >> 2;
        int suite = path[i] & 3;
        ++nominal_stat[nominal];
        ++suite_stat[suite];
    }

    for (int n=0; n<13; ++n) {
        ++stat[nominal_stat[n]];
    }

    int possible_suite = -1;
    for (int i=0; i<4; ++i) {
        if (suite_stat[i] >= 3) {
            possible_suite = i;
            break;
        }
    }

    if (possible_suite >= 0) {
        uint64_t flush_mask = 0;
        int c[2] = { 0 };
        int counter = 0;
        for (int i=0; i<5; ++i) {
            int nominal = path[i] >> 2;
            if ((path[i] & 3) == possible_suite) {
                flush_mask |= 1ull << nominal;
            } else {
                c[counter++] = nominal;
            }
        }

        if (c[0] < c[1]) {
            int tmp = c[0];
            c[0] = c[1];
            c[1] = tmp;
        }

        return 0
            | FLUSH
            | flush_mask
            | (possible_suite << 14)
            | (c[0] << 16)
            | (c[1] << 20)
        ;
    }

    uint64_t rank = 0;
    int stats[5] = { 0 };
    for (int n=0; n<13; ++n) {
        if (nominal_stat[n] == 0) continue;
        ++stats[nominal_stat[n]];
        int shift = n + 13*nominal_stat[n] - 13;
        rank |= 1ull << shift;
    }

    uint64_t prefix = 0;

    if (0) ;
    else if (stats[4] == 1 && stats[3] == 0 && stats[2] == 0 && stats[1] == 1) prefix = FOUR_OF_A_KIND;
    else if (stats[4] == 0 && stats[3] == 1 && stats[2] == 1 && stats[1] == 0) prefix = FULL_HOUSE;
    else if (stats[4] == 0 && stats[3] == 1 && stats[2] == 0 && stats[1] == 2) prefix = THREE_OF_A_KIND;
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 2 && stats[1] == 1) prefix = TWO_PAIR;
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 1 && stats[1] == 3) prefix = ONE_PAIR;
    else if (stats[4] == 0 && stats[3] == 0 && stats[2] == 0 && stats[1] == 5) prefix = HIGH_CARD;
    else {
        fprintf(stderr, "Assertion failed: invalid stats array: %d, %d, %d, %d, %d.\n", stats[0], stats[1], stats[2], stats[3], stats[4]);
        abort();
    }

    return rank | prefix;
}

void build_omaha7_script(void * script)
{
    script_step_comb(script, 52, 5);
    script_step_pack(script, omaha_simplify_5, 0);
    script_step_comb(script, 52, 2);
}


struct selector
{
    const char * name;
    build_script_func * build;
    check_ofsm_func * check;
};

struct selector selectors[] = {
    { "omaha7", build_omaha7_script, NULL },
    { "holdem5", build_holdem5_script, check_holdem5 },
    { NULL, NULL }
};

int main(int argc, char * argv[])
{
    const struct selector * selector = selectors;
    for (; selector->name != NULL; ++selector) {
        if (is_prefix(selector->name, &argc, argv)) {
            return execute(argc, argv, selector->build, selector->check);
        }
    }

    selector = selectors;
    for (; selector->name != NULL; ++selector) {
        printf("%s\n", selector->name);
    }

    return 0;
}
