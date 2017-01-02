#include "poker.h"

#define NOMINAL_A     12
#define NOMINAL_K     11
#define NOMINAL_Q     10
#define NOMINAL_J      9
#define NOMINAL_T      8
#define NOMINAL_9      7
#define NOMINAL_8      6
#define NOMINAL_7      5
#define NOMINAL_6      4
#define NOMINAL_5      3
#define NOMINAL_4      2
#define NOMINAL_3      1
#define NOMINAL_2      0

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
#define HAND_TYPE__THREE_OF_A_KIND      4
#define HAND_TYPE__STRAIGHT             5
#define HAND_TYPE__FLUSH                6
#define HAND_TYPE__FULL_HOUSE           7
#define HAND_TYPE__FOUR_OF_A_KIND       8
#define HAND_TYPE__STRAIGHT_FLUSH       9

#define HAND_TYPE__FIRST_BIT           56
#define PREFIX(hand_type)     ((uint64_t)HAND_TYPE__##hand_type << HAND_TYPE__FIRST_BIT)



const char * nominal52_str = "23456789TJQKA";
const char * card52_str[] = {
    "2h", "2d", "2c", "2s",
    "3h", "3d", "3c", "3s",
    "4h", "4d", "4c", "4s",
    "5h", "5d", "5c", "5s",
    "6h", "6d", "6c", "6s",
    "7h", "7d", "7c", "7s",
    "8h", "8d", "8c", "8s",
    "9h", "9d", "9c", "9s",
    "Th", "Td", "Tc", "Ts",
    "Jh", "Jd", "Jc", "Js",
    "Qh", "Qd", "Qc", "Qs",
    "Kh", "Kd", "Kc", "Ks",
    "Ah", "Ad", "Ac", "As"
};



uint64_t eval_rank5_via_robust_for_deck52(const card_t * cards)
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

const card_t quick_ordered_hand5_for_deck52[] = {
    CARD_2h, CARD_3c, CARD_4c, CARD_5c, CARD_7s,
    CARD_3h, CARD_5d, CARD_7c, CARD_9c, CARD_Js,
/*
    CARD_6s, CARD_6c, CARD_9c, CARD_Qd, CARD_As,
    CARD_Ts, CARD_Tc, CARD_6c, CARD_7d, CARD_8c,
    CARD_7s, CARD_7d, CARD_6s, CARD_6c, CARD_Ad,
    CARD_As, CARD_Ah, CARD_Kh, CARD_Kc, CARD_Tc,
    CARD_As, CARD_9c, CARD_8d, CARD_7h, CARD_6h,
    CARD_6c, CARD_7c, CARD_8c, CARD_9s, CARD_Ts,
    CARD_Ad, CARD_Kc, CARD_Qs, CARD_Js, CARD_Ts,
    CARD_Jc, CARD_Jd, CARD_Jh, CARD_9c, CARD_7h,
    CARD_Ks, CARD_Kc, CARD_Kd, CARD_Qs, CARD_6d,
    CARD_6s, CARD_6c, CARD_6h, CARD_As, CARD_Ad,
    CARD_Js, CARD_Jd, CARD_Jh, CARD_9s, CARD_9d,
    CARD_As, CARD_Qs, CARD_Ts, CARD_7s, CARD_6s,
    CARD_Ad, CARD_Kd, CARD_Td, CARD_9d, CARD_7d,
    CARD_8c, CARD_8s, CARD_8d, CARD_8h, CARD_6c,
    CARD_8s, CARD_8c, CARD_8d, CARD_8h, CARD_Kc,
    CARD_Ad, CARD_9d, CARD_8d, CARD_7d, CARD_6d,
    CARD_Tc, CARD_9c, CARD_8c, CARD_7c, CARD_6c,
    CARD_Kh, CARD_Qh, CARD_Jh, CARD_Th, CARD_9h,
    CARD_Ah, CARD_Kh, CARD_Qh, CARD_Jh, CARD_Th,
*/
    0xFF
};

const card_t quick_ordered_hand7_for_deck52[] = {
    CARD_6h, CARD_7c, CARD_8c, CARD_9c, CARD_Js, CARD_Qs, CARD_Kd,
    CARD_6h, CARD_7d, CARD_8c, CARD_Tc, CARD_Qd, CARD_Kd, CARD_As,
    CARD_6s, CARD_6c, CARD_9c, CARD_Js, CARD_Qd, CARD_Kd, CARD_As,
    CARD_Ts, CARD_Tc, CARD_6c, CARD_7d, CARD_8c, CARD_Js, CARD_Ks,
    CARD_7s, CARD_7d, CARD_6s, CARD_6c, CARD_9h, CARD_Th, CARD_Ad,
    CARD_As, CARD_Ah, CARD_Kh, CARD_Kc, CARD_Tc, CARD_7d, CARD_6h,
    CARD_As, CARD_9c, CARD_8d, CARD_7h, CARD_6h, CARD_Kd, CARD_Qd,
    CARD_6c, CARD_7c, CARD_8c, CARD_9s, CARD_Ts, CARD_Kd, CARD_Qd,
    CARD_Ad, CARD_Kc, CARD_Qs, CARD_Js, CARD_Ts, CARD_7h, CARD_6h,
    CARD_Jc, CARD_Jd, CARD_Jh, CARD_9c, CARD_8h, CARD_7h, CARD_6d,
    CARD_Ks, CARD_Kc, CARD_Kd, CARD_Qs, CARD_9d, CARD_8h, CARD_6s,
    CARD_6s, CARD_6c, CARD_6h, CARD_As, CARD_Ad, CARD_Kh, CARD_Qs,
    CARD_Js, CARD_Jd, CARD_Jh, CARD_9s, CARD_9d, CARD_6c, CARD_Tc,
    CARD_As, CARD_Qs, CARD_Ts, CARD_7s, CARD_6s, CARD_8d, CARD_9d,
    CARD_Ad, CARD_Kd, CARD_Td, CARD_9d, CARD_7d, CARD_7h, CARD_9c,
    CARD_8c, CARD_8s, CARD_8d, CARD_8h, CARD_6c, CARD_7h, CARD_Js,
    CARD_8s, CARD_8c, CARD_8d, CARD_8h, CARD_Kc, CARD_6s, CARD_6s,
    CARD_Ad, CARD_9d, CARD_8d, CARD_7d, CARD_6d, CARD_Jd, CARD_Qd,
    CARD_Tc, CARD_9c, CARD_8c, CARD_7c, CARD_6c, CARD_6h, CARD_6s,
    CARD_Kh, CARD_Qh, CARD_Jh, CARD_Th, CARD_9h, CARD_6s, CARD_6h,
    CARD_Ah, CARD_Kh, CARD_Qh, CARD_Jh, CARD_Th, CARD_9h, CARD_8h,
    0xFF
};
