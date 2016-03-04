#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef uint32_t card_t;

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

static uint64_t eval_rank5_via_slow_robust(const card_t * cards)
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

const uint32_t * six_plus_fsm5;
const uint32_t * six_plus_fsm7;

static inline uint32_t eval_rank5_via_fsm5(const card_t * cards)
{
    uint32_t current = 52;
    current = six_plus_fsm5[current + cards[0]];
    current = six_plus_fsm5[current + cards[1]];
    current = six_plus_fsm5[current + cards[2]];
    current = six_plus_fsm5[current + cards[3]];
    current = six_plus_fsm5[current + cards[4]];
    return current;
}

static inline int eval_rank7_via_fsm5_brutte(const card_t * cards)
{
    #define NEW_WAY(p, n1, n2, n3, n4) \
        { \
            uint32_t current = p; \
            current = six_plus_fsm5[current + cards[n1]]; \
            current = six_plus_fsm5[current + cards[n2]]; \
            current = six_plus_fsm5[current + cards[n3]]; \
            current = six_plus_fsm5[current + cards[n4]]; \
            if (current > result) result = current; \
        }

    uint32_t result = 0;
    uint32_t start = 52;

    uint32_t s0 = six_plus_fsm5[start + cards[0]];
    uint32_t s1 = six_plus_fsm5[start + cards[1]];
    uint32_t s2 = six_plus_fsm5[start + cards[2]];

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

static inline uint32_t eval_rank7_via_fsm5_opt(const card_t * cards)
{
    uint32_t result = 0;
    uint32_t start = 52;

    uint32_t s0 = six_plus_fsm5[start + cards[0]];
    uint32_t s1 = six_plus_fsm5[start + cards[1]];
    uint32_t s2 = six_plus_fsm5[start + cards[2]];

    #define NEW_STATE(x, y) uint32_t x##y = six_plus_fsm5[x + cards[y]];
    #define LAST_STATE(x, y) { uint32_t tmp = six_plus_fsm5[x + cards[y]]; if (tmp > result) result = tmp; }

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

static inline uint32_t eval_rank7_via_fsm7(const card_t * cards)
{
    uint32_t current = 52;
    current = six_plus_fsm7[current + cards[0]];
    current = six_plus_fsm7[current + cards[1]];
    current = six_plus_fsm7[current + cards[2]];
    current = six_plus_fsm7[current + cards[3]];
    current = six_plus_fsm7[current + cards[4]];
    current = six_plus_fsm7[current + cards[5]];
    current = six_plus_fsm7[current + cards[6]];
    return current;
}

#include "ofsm.h"

#define FILENAME_FSM5  "six-plus-5.bin"
#define SIGNATURE_FSM5 "OFSM Six Plus 5"

void save_binary(const char * file_name, const char * name, const struct ofsm_array * array);

static void load_fsm5()
{
    if (six_plus_fsm5 != NULL) return;

    FILE * f = fopen(FILENAME_FSM5, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: Can not open file “%s”, error %d, %s\n.", FILENAME_FSM5, errno, strerror(errno));
        errno = 0;
        return;
    }

    struct array_header header;
    size_t sz = sizeof(struct array_header);
    size_t was_read = fread(&header, 1, sz, f);
    if (was_read != sz) {
        fprintf(stderr, "Error: Can not read %lu bytes from file “%s”", sz, FILENAME_FSM5);
        if (errno != 0) {
            fprintf(stderr, ", error %d, %s", errno, strerror(errno));
            errno = 0;
        }
        if (feof(f)) {
            fprintf(stderr, ", unexpected EOF");
        }
        fprintf(stderr, ".\n");
        fclose(f);
        return;
    }

    if (strncmp(header.name, SIGNATURE_FSM5, 16) != 0) {
        fclose(f);
        fprintf(stderr, "Error: Wrong signature for file “%s”.\n", FILENAME_FSM5);
        fprintf(stderr, "  Header:   “%*s”\n", 16, header.name);
        fprintf(stderr, "  Expected: “%s”\n", SIGNATURE_FSM5);
        return;
    }

    if (header.start_from != 36) {
        fclose(f);
        fprintf(stderr, "Error: Wrong start_from value for file “%s”.\n", FILENAME_FSM5);
        fprintf(stderr, "  Header:   “%u”\n", header.start_from);
        fprintf(stderr, "  Expected: “%u”\n", 16);
        return;
    }

    if (header.qflakes != 5) {
        fclose(f);
        fprintf(stderr, "Error: Wrong qflakes value for file “%s”.\n", FILENAME_FSM5);
        fprintf(stderr, "  Header:   “%u”\n", header.qflakes);
        fprintf(stderr, "  Expected: “%u”\n", 5);
        return;
    }

    sz = header.len * sizeof(uint32_t);
    void * ptr = malloc(sz);
    if (ptr == NULL) {
        fclose(f);
        fprintf(stderr, "Error: allocation error, malloc(%lu) failed with a NULL as a result.\n", sz);
        return;
    }

    was_read = fread(ptr, 1, sz, f);

    if (was_read == sz) {
        six_plus_fsm5 = ptr;
    } else {
        fprintf(stderr, "Error: Can not read %lu bytes from file “%s”", sz, FILENAME_FSM5);
        if (errno != 0) {
            fprintf(stderr, ", error %d, %s", errno, strerror(errno));
            errno = 0;
        }
        if (feof(f)) {
            fprintf(stderr, ", unexpected EOF");
        }
        fprintf(stderr, ".\n");
        free(ptr);
    }

    fclose(f);
}

pack_value_t calc_six_plus_5(unsigned int n, const input_t * path)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 5, but %u as passed.\n", n);
        exit(1);
    }

    card_t cards[n];
    for (size_t i=0; i<n; ++i) {
        cards[i] = path[i];
    }

    return eval_rank5_via_slow_robust(cards);
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

pack_value_t calc_six_plus_7(unsigned int n, const input_t * path)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_7 requires n = 7, but %u as passed.\n", n);
        abort();
    }

    card_t cards[n];
    for (size_t i=0; i<n; ++i) {
        cards[i] = path[i];
    }

    return eval_rank5_via_fsm5(cards);
}

void build_six_plus_7(void * script)
{
    load_fsm5();
    if (six_plus_fsm5 == NULL) {
        return;
    }

    script_step_comb(script, 36, 7);
    // Forget suites
    script_step_pack(script, calc_six_plus_7, 0);
    script_step_optimize(script, 7, NULL);
    script_step_optimize(script, 6, NULL);
    script_step_optimize(script, 5, NULL);
    script_step_optimize(script, 4, NULL);
    script_step_optimize(script, 3, NULL);
    script_step_optimize(script, 2, NULL);
    script_step_optimize(script, 1, NULL);
}

int check_six_plus_7(const void * ofsm)
{
    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 1, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    // ofsm_print_array(stdout, "fsm5_data", &array, 52);
    save_binary("six-plus-7.bin", "OFSM Six Plus 7", &array);

    free(array.array);
    return 0;
}
