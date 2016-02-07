#ifndef YOO__POKER__H__
#define YOO__POKER__H__

#include <stddef.h>
#include <stdint.h>



/* Constants */

#define NOMINAL_A    12
#define NOMINAL_K    11
#define NOMINAL_Q    10
#define NOMINAL_J     9
#define NOMINAL_T     8
#define NOMINAL_9     7
#define NOMINAL_8     6
#define NOMINAL_7     5
#define NOMINAL_6     4
#define NOMINAL_5     3
#define NOMINAL_4     2
#define NOMINAL_3     1
#define NOMINAL_2     0

#define SUITE_S    3
#define SUITE_C    2
#define SUITE_D    1
#define SUITE_H    0

#define CARD(nominal, suite)  (nominal*4 + suite)
#define SUITE(card)           (card & 3)
#define NOMINAL(card)         (card >> 2)

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

#define CMASK(c) (1ull << CARD_##c)
#define CMASK_2(c1, c2) (CMASK(c1) | CMASK(c2))
#define CMASK_3(c1, c2, c3) (CMASK_2(c1, c2) | CMASK(c3))

#define NOMINAL_MASK(n)  (1 << NOMINAL_##n)
#define NOMINAL_MASK_2(n1, n2)  (NOMINAL_MASK(n1) | NOMINAL_MASK(n2))
#define NOMINAL_MASK_3(n1, n2, n3)  (NOMINAL_MASK_2(n1, n2) | NOMINAL_MASK(n3))
#define NOMINAL_MASK_4(n1, n2, n3, n4)  (NOMINAL_MASK_3(n1, n2, n3) | NOMINAL_MASK(n4))
#define NOMINAL_MASK_5(n1, n2, n3, n4, n5)  (NOMINAL_MASK_4(n1, n2, n3, n4) | NOMINAL_MASK(n5))

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

#define UNIQUE_STAT_5__TOTAL             2598960
#define UNIQUE_STAT_5__HIGH_CARD         1302540
#define UNIQUE_STAT_5__ONE_PAIR          1098240
#define UNIQUE_STAT_5__TWO_PAIR           123552
#define UNIQUE_STAT_5__THREE_OF_A_KIND     54912
#define UNIQUE_STAT_5__STRAIGHT            10200
#define UNIQUE_STAT_5__FLUSH                5108
#define UNIQUE_STAT_5__FULL_HOUSE           3744
#define UNIQUE_STAT_5__FOUR_OF_A_KIND        624
#define UNIQUE_STAT_5__STRAIGHT_FLUSH         40

#define NRANK__HIGH_CARD_MIN                                           (1)
#define NRANK__ONE_PAIR_MIN                  (NRANK__HIGH_CARD_MIN + 1277)
#define NRANK__TWO_PAIRS_MIN                  (NRANK__ONE_PAIR_MIN + 2860)
#define NRANK__THREE_OF_KIND_MIN             (NRANK__TWO_PAIRS_MIN +  858)
#define NRANK__STRAIGHT_MIN              (NRANK__THREE_OF_KIND_MIN +  858)
#define NRANK__FLUSH_MIN                      (NRANK__STRAIGHT_MIN +   10)
#define NRANK__FULL_HOUSES_MIN                   (NRANK__FLUSH_MIN + 1277)
#define NRANK__FOUR_OF_KIND_MIN            (NRANK__FULL_HOUSES_MIN +  156)
#define NRANK__STRAIGHT_FLUSH_MIN         (NRANK__FOUR_OF_KIND_MIN +  156)
#define NRANK__MAX                      (NRANK__STRAIGHT_FLUSH_MIN +   10)



/* Common */

typedef int card_t;

extern const char * card_str[];
extern const char * nominal_str;
extern const char * suite_str;

extern const int * const manual_fsm5;
extern const int * const fsm5;
extern const int * const fsm7;

const card_t * deal_cards(int n, card_t * restrict deck, int deck_size);



/* Hand evaluator equivalence */

#define HEE__NO_MEMORY                   -2
#define HEE__DIFFERENT_PAIR               1
#define HEE__NOT_EQUIVALENT               2
#define HEE__MAX_HASH_CONFLICT_EXCEEDED   3

struct hand_evaluator_equivalence
{
    size_t mod;
    size_t max_hash_conflict;

    size_t plane_count;
    size_t unique_pairs;
    size_t * restrict sizes;
    uint64_t * restrict * restrict planes;
};

struct hand_evaluator_equivalence * hee_create(size_t mod, size_t max_hash_conflict);
void hee_destroy(struct hand_evaluator_equivalence * restrict me);
int hee_add_pair(struct hand_evaluator_equivalence * restrict me, uint64_t v1, uint64_t v2);
int hee_verify(struct hand_evaluator_equivalence * restrict me);



/* Slow robus hand evaluation */

uint64_t eval_slow_robust_rank5(const card_t * cards);

static inline int eval_manual_fsm_nrank5(const card_t * cards)
{
    int current = 52;
    current = manual_fsm5[current + cards[0]];
    current = manual_fsm5[current + cards[1]];
    current = manual_fsm5[current + cards[2]];
    current = manual_fsm5[current + cards[3]];
    current = manual_fsm5[current + cards[4]];
    return current;
}

static inline int eval_fsm_nrank5(const card_t * cards)
{
    int current = 52;
    current = fsm5[current + cards[0]];
    current = fsm5[current + cards[1]];
    current = fsm5[current + cards[2]];
    current = fsm5[current + cards[3]];
    current = fsm5[current + cards[4]];
    return current;
}

static inline int eval_slow_enum_nrank7(const card_t * cards)
{
    #define NEW_WAY(p, n1, n2, n3, n4) \
        { \
            int current = p; \
            current = fsm5[current + cards[n1]]; \
            current = fsm5[current + cards[n2]]; \
            current = fsm5[current + cards[n3]]; \
            current = fsm5[current + cards[n4]]; \
            if (current > result) result = current; \
        }

    int result = 0;
    int start = 52;

    int s0 = fsm5[start + cards[0]];
    int s1 = fsm5[start + cards[1]];
    int s2 = fsm5[start + cards[2]];

    NEW_WAY(s0, 1, 2, 3, 4);
    NEW_WAY(s0, 1, 2, 3, 5);
    NEW_WAY(s0, 1, 2, 3, 6);
    NEW_WAY(s0, 1, 2, 4, 5);
    NEW_WAY(s0, 1, 2, 4, 6);
    NEW_WAY(s0, 1, 2, 5, 6);
    NEW_WAY(s0, 1, 3, 4, 5);
    NEW_WAY(s0, 1, 3, 4, 6);
    NEW_WAY(s0, 1, 3, 5, 6);
    NEW_WAY(s0, 1, 4, 5, 6);
    NEW_WAY(s0, 2, 3, 4, 5);
    NEW_WAY(s0, 2, 3, 4, 6);
    NEW_WAY(s0, 2, 3, 5, 6);
    NEW_WAY(s0, 2, 4, 5, 6);
    NEW_WAY(s0, 3, 4, 5, 6);
    NEW_WAY(s1, 2, 3, 4, 5);
    NEW_WAY(s1, 2, 3, 4, 6);
    NEW_WAY(s1, 2, 3, 5, 6);
    NEW_WAY(s1, 2, 4, 5, 6);
    NEW_WAY(s1, 3, 4, 5, 6);
    NEW_WAY(s2, 3, 4, 5, 6);

    #undef NEW_WAY

    return result;
}

static inline int eval_enum_nrank7(const card_t * cards)
{
    int result = 0;
    int start = 52;

    int s0 = fsm5[start + cards[0]];
    int s1 = fsm5[start + cards[1]];
    int s2 = fsm5[start + cards[2]];

    #define NEW_STATE(x, y) int x##y = fsm5[x + cards[y]];
    #define LAST_STATE(x, y) { int tmp = fsm5[x + cards[y]]; if (tmp > result) result = tmp; }

    NEW_STATE(s0, 1);
    NEW_STATE(s0, 2);
    NEW_STATE(s0, 3);
    NEW_STATE(s1, 2);
    NEW_STATE(s1, 3);
    NEW_STATE(s2, 3);

    NEW_STATE(s01, 2);
    NEW_STATE(s01, 3);
    NEW_STATE(s01, 4);
    NEW_STATE(s02, 3);
    NEW_STATE(s02, 4);
    NEW_STATE(s03, 4);
    NEW_STATE(s12, 3);
    NEW_STATE(s12, 4);
    NEW_STATE(s13, 4);
    NEW_STATE(s23, 4);

    NEW_STATE(s012, 3);
    NEW_STATE(s012, 4);
    NEW_STATE(s012, 5);
    NEW_STATE(s013, 4);
    NEW_STATE(s013, 5);
    NEW_STATE(s014, 5);
    NEW_STATE(s023, 4);
    NEW_STATE(s023, 5);
    NEW_STATE(s024, 5);
    NEW_STATE(s034, 5);
    NEW_STATE(s123, 4);
    NEW_STATE(s123, 5);
    NEW_STATE(s124, 5);
    NEW_STATE(s134, 5);
    NEW_STATE(s234, 5);

    LAST_STATE(s0123, 4);
    LAST_STATE(s0123, 5);
    LAST_STATE(s0123, 6);
    LAST_STATE(s0124, 5);
    LAST_STATE(s0124, 6);
    LAST_STATE(s0125, 6);
    LAST_STATE(s0134, 5);
    LAST_STATE(s0134, 6);
    LAST_STATE(s0135, 6);
    LAST_STATE(s0145, 6);
    LAST_STATE(s0234, 5);
    LAST_STATE(s0234, 6);
    LAST_STATE(s0235, 6);
    LAST_STATE(s0245, 6);
    LAST_STATE(s0345, 6);
    LAST_STATE(s1234, 5);
    LAST_STATE(s1234, 6);
    LAST_STATE(s1235, 6);
    LAST_STATE(s1245, 6);
    LAST_STATE(s1345, 6);
    LAST_STATE(s2345, 6);

    #undef NEW_STATE
    #undef LAST_STATE

    return result;
}

static inline int eval_fsm_nrank7(const card_t * cards)
{
    int current = 52;
    current = fsm7[current + cards[0]];
    current = fsm7[current + cards[1]];
    current = fsm7[current + cards[2]];
    current = fsm7[current + cards[3]];
    current = fsm7[current + cards[4]];
    current = fsm7[current + cards[5]];
    current = fsm7[current + cards[6]];
    return current;
}


/* Tests */

typedef int (* test_f)();

struct test_item
{
    const char * name;
    test_f func;
    int is_slow;
};

extern struct test_item * current_test;

#endif
