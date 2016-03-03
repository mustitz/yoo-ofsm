#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CARD(nominal, suite)  (nominal*4 + suite)
#define SUITE(card)           (card & 3)
#define NOMINAL(card)         (card >> 2)

#define NOMINAL_A     8
#define NOMINAL_K     7
#define NOMINAL_Q     6
#define NOMINAL_J     5
#define NOMINAL_T     4
#define NOMINAL_9     3
#define NOMINAL_8     2
#define NOMINAL_7     1
#define NOMINAL_6     0

#define NOMINAL_MASK(n)  (1 << NOMINAL_##n)
#define NOMINAL_MASK_2(n1, n2)  (NOMINAL_MASK(n1) | NOMINAL_MASK(n2))
#define NOMINAL_MASK_3(n1, n2, n3)  (NOMINAL_MASK_2(n1, n2) | NOMINAL_MASK(n3))
#define NOMINAL_MASK_4(n1, n2, n3, n4)  (NOMINAL_MASK_3(n1, n2, n3) | NOMINAL_MASK(n4))
#define NOMINAL_MASK_5(n1, n2, n3, n4, n5)  (NOMINAL_MASK_4(n1, n2, n3, n4) | NOMINAL_MASK(n5))

#define SUITE_S    3
#define SUITE_C    2
#define SUITE_D    1
#define SUITE_H    0

#define CARD_As    CARD(NOMINAL_A, SUITE_S)
#define CARD_Ac    CARD(NOMINAL_A, SUITE_C)
#define CARD_Ad    CARD(NOMINAL_A, SUITE_D)
#define CARD_Ah    CARD(NOMINAL_A, SUITE_H)

#define CARD_Ks    CARD(NOMINAL_K, SUITE_S)
#define CARD_Kc    CARD(NOMINAL_K, SUITE_C)
#define CARD_Kd    CARD(NOMINAL_K, SUITE_D)
#define CARD_Kh    CARD(NOMINAL_K, SUITE_H)

#define CARD_Qs    CARD(NOMINAL_Q, SUITE_S)
#define CARD_Qc    CARD(NOMINAL_Q, SUITE_C)
#define CARD_Qd    CARD(NOMINAL_Q, SUITE_D)
#define CARD_Qh    CARD(NOMINAL_Q, SUITE_H)

#define CARD_Js    CARD(NOMINAL_J, SUITE_S)
#define CARD_Jc    CARD(NOMINAL_J, SUITE_C)
#define CARD_Jd    CARD(NOMINAL_J, SUITE_D)
#define CARD_Jh    CARD(NOMINAL_J, SUITE_H)

#define CARD_Ts    CARD(NOMINAL_T, SUITE_S)
#define CARD_Tc    CARD(NOMINAL_T, SUITE_C)
#define CARD_Td    CARD(NOMINAL_T, SUITE_D)
#define CARD_Th    CARD(NOMINAL_T, SUITE_H)

#define CARD_9s    CARD(NOMINAL_9, SUITE_S)
#define CARD_9c    CARD(NOMINAL_9, SUITE_C)
#define CARD_9d    CARD(NOMINAL_9, SUITE_D)
#define CARD_9h    CARD(NOMINAL_9, SUITE_H)

#define CARD_8s    CARD(NOMINAL_8, SUITE_S)
#define CARD_8c    CARD(NOMINAL_8, SUITE_C)
#define CARD_8d    CARD(NOMINAL_8, SUITE_D)
#define CARD_8h    CARD(NOMINAL_8, SUITE_H)

#define CARD_7s    CARD(NOMINAL_7, SUITE_S)
#define CARD_7c    CARD(NOMINAL_7, SUITE_C)
#define CARD_7d    CARD(NOMINAL_7, SUITE_D)
#define CARD_7h    CARD(NOMINAL_7, SUITE_H)

#define CARD_6s    CARD(NOMINAL_6, SUITE_S)
#define CARD_6c    CARD(NOMINAL_6, SUITE_C)
#define CARD_6d    CARD(NOMINAL_6, SUITE_D)
#define CARD_6h    CARD(NOMINAL_6, SUITE_H)

#define CARD_5s    CARD(NOMINAL_5, SUITE_S)
#define CARD_5c    CARD(NOMINAL_5, SUITE_C)
#define CARD_5d    CARD(NOMINAL_5, SUITE_D)
#define CARD_5h    CARD(NOMINAL_5, SUITE_H)

#define CARD_4s    CARD(NOMINAL_4, SUITE_S)
#define CARD_4c    CARD(NOMINAL_4, SUITE_C)
#define CARD_4d    CARD(NOMINAL_4, SUITE_D)
#define CARD_4h    CARD(NOMINAL_4, SUITE_H)

#define CARD_3s    CARD(NOMINAL_3, SUITE_S)
#define CARD_3c    CARD(NOMINAL_3, SUITE_C)
#define CARD_3d    CARD(NOMINAL_3, SUITE_D)
#define CARD_3h    CARD(NOMINAL_3, SUITE_H)

#define CARD_2s    CARD(NOMINAL_2, SUITE_S)
#define CARD_2c    CARD(NOMINAL_2, SUITE_C)
#define CARD_2d    CARD(NOMINAL_2, SUITE_D)
#define CARD_2h    CARD(NOMINAL_2, SUITE_H)

#define HAND_TYPE__HIGH_CARD            1
#define HAND_TYPE__ONE_PAIR             2
#define HAND_TYPE__TWO_PAIR             3
#define HAND_TYPE__STRAIGHT             4
#define HAND_TYPE__THREE_OF_A_KIND      5
#define HAND_TYPE__FULL_HOUSE           6
#define HAND_TYPE__FLUSH                7
#define HAND_TYPE__FOUR_OF_A_KIND       8
#define HAND_TYPE__STRAIGHT_FLUSH       9

#define HAND_TYPE__FIRST_BIT           56
#define PREFIX(hand_type)     ((uint64_t)HAND_TYPE__##hand_type << HAND_TYPE__FIRST_BIT)

typedef int card_t;

const char * card36_str[] = {
    "6s", "6c", "6d", "6h",
    "7s", "7c", "7d", "7h",
    "8s", "8c", "8d", "8h",
    "9s", "9c", "9d", "9h",
    "Ts", "Tc", "Td", "Th",
    "Js", "Jc", "Jd", "Jh",
    "Qs", "Qc", "Qd", "Qh",
    "Ks", "Kc", "Kd", "Kh",
    "As", "Ac", "Ad", "Ah"
};

const char * nominal36_str = "6789TJQKA";
const char * suite36_str = "hdcs";

static uint64_t eval_slow_robust_rank5(const card_t * cards)
{
    int nominal_stat[9] = { 0 };
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
        || nominal_mask == NOMINAL_MASK_5(9,8,7,6,A)
    ;

    if (is_straight) {
        if (nominal_mask == NOMINAL_MASK_5(9,8,7,6,A)) {
            nominal_mask = NOMINAL_MASK_4(9,8,7,6);
        }

        uint64_t prefix = is_flush ? PREFIX(STRAIGHT_FLUSH) : PREFIX(STRAIGHT);
        return prefix | nominal_mask;
    }

    if (is_flush) {
        return PREFIX(FLUSH) | nominal_mask;
    }

    uint64_t rank = 0;
    int stats[5] = { 0 };
    for (int n=NOMINAL_6; n<=NOMINAL_A; ++n) {
        if (nominal_stat[n] == 0) continue;
        ++stats[nominal_stat[n]];
        int shift = n + 9*nominal_stat[n] - 9;
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

#include "ofsm.h"

void save_binary(const char * file_name, const char * name, const struct ofsm_array * array);

pack_value_t calc_six_plus_5(unsigned int n, const input_t * path)
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

void build_six_plus_5(void * script)
{
    script_step_comb(script, 36, 5);
    script_step_pack(script, calc_six_plus_5, 0);
    script_step_optimize(script, 5, NULL);
    script_step_optimize(script, 4, NULL);
    script_step_optimize(script, 3, NULL);
    script_step_optimize(script, 2, NULL);
    script_step_optimize(script, 1, NULL);
}

int check_six_plus_5(const void * ofsm)
{
    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    // ofsm_print_array(stdout, "fsm5_data", &array, 52);
    save_binary("six-plus-5.bin", "OFSM Six Plus 5", &array);

    free(array.array);
    return 0;
}
