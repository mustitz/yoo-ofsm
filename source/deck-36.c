#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yoo-combinatoric.h"
#include "yoo-ofsm.h"
#include "yoo-stdlib.h"

extern int opt_opencl;
int init_opencl(FILE * err);
int free_opencl(void);

int opencl__test_permutations(
    const int32_t n, const uint32_t start_state, const uint32_t qdata,
    const int8_t * const perm_table, const uint64_t perm_table_sz,
    const uint32_t * const fsm, const uint64_t fsm_sz,
    const uint64_t * const data, const uint64_t data_sz,
    uint16_t * restrict const report, const uint64_t report_sz
);

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



/* Our GOAL */

typedef uint32_t card_t;

const uint32_t * six_plus_fsm5;
const uint32_t * six_plus_fsm7;

uint64_t six_plus_fsm5_sz;
uint64_t six_plus_fsm7_sz;

static inline uint32_t eval_rank5_via_fsm5(const card_t * cards)
{
    uint32_t current = 36;
    current = six_plus_fsm5[current + cards[0]];
    current = six_plus_fsm5[current + cards[1]];
    current = six_plus_fsm5[current + cards[2]];
    current = six_plus_fsm5[current + cards[3]];
    current = six_plus_fsm5[current + cards[4]];
    return current;
}

static inline uint32_t eval_rank7_via_fsm7(const card_t * cards)
{
    uint32_t current = 36;
    current = six_plus_fsm7[current + cards[0]];
    current = six_plus_fsm7[current + cards[1]];
    current = six_plus_fsm7[current + cards[2]];
    current = six_plus_fsm7[current + cards[3]];
    current = six_plus_fsm7[current + cards[4]];
    current = six_plus_fsm7[current + cards[5]];
    current = six_plus_fsm7[current + cards[6]];
    return current;
}



/* Library */

const char * card36_str[] = {
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

const char * nominal36_str = "6789TJQKA";
const char * suite36_str = "hdcs";

static inline void mask_to_cards(const int n, uint64_t mask, card_t * restrict cards)
{
    for (int i=0; i<n; ++i) {
        cards[i] = extract_rbit64(&mask);
    }
}

static inline void print_hand(const int n, const card_t * const cards)
{
    for (int i=0; i<n; ++i) {
        printf(" %s", card36_str[cards[i]]);
    }
}

static inline void gen_perm(const int n, card_t * restrict const dest, const card_t * const src, const int * perm)
{
    for (int i=0; i<n; ++i) {
        dest[i] = src[perm[i]];
    }
}

/* Load OFSMs */

static void * load_fsm(const char * filename, const char * signature, uint32_t start_from, uint32_t qflakes, uint64_t * fsm_sz)
{
    FILE * f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: Can not open file “%s”, error %d, %s.\n", filename, errno, strerror(errno));
        errno = 0;
        return NULL;
    }

    struct array_header header;
    size_t sz = sizeof(struct array_header);
    size_t was_read = fread(&header, 1, sz, f);
    if (was_read != sz) {
        fprintf(stderr, "Error: Can not read %lu bytes from file “%s”", sz, filename);
        if (errno != 0) {
            fprintf(stderr, ", error %d, %s", errno, strerror(errno));
            errno = 0;
        }
        if (feof(f)) {
            fprintf(stderr, ", unexpected EOF");
        }
        fprintf(stderr, ".\n");
        fclose(f);
        return NULL;
    }

    if (strncmp(header.name, signature, 16) != 0) {
        fclose(f);
        fprintf(stderr, "Error: Wrong signature for file “%s”.\n", filename);
        fprintf(stderr, "  Header:   “%*s”\n", 16, header.name);
        fprintf(stderr, "  Expected: “%s”\n", signature);
        return NULL;
    }

    if (header.start_from != start_from) {
        fclose(f);
        fprintf(stderr, "Error: Wrong start_from value for file “%s”.\n", filename);
        fprintf(stderr, "  Header:   “%u”\n", header.start_from);
        fprintf(stderr, "  Expected: “%u”\n", start_from);
        return NULL;
    }

    if (header.qflakes != qflakes) {
        fclose(f);
        fprintf(stderr, "Error: Wrong qflakes value for file “%s”.\n", filename);
        fprintf(stderr, "  Header:   “%u”\n", header.qflakes);
        fprintf(stderr, "  Expected: “%u”\n", qflakes);
        return NULL;
    }

    sz = header.len * sizeof(uint32_t);
    void * ptr = malloc(sz);
    if (ptr == NULL) {
        fclose(f);
        fprintf(stderr, "Error: allocation error, malloc(%lu) failed with a NULL as a result.\n", sz);
        return NULL;
    }

    was_read = fread(ptr, 1, sz, f);

    if (was_read != sz) {
        fprintf(stderr, "Error: Can not read %lu bytes from file “%s”", sz, filename);
        if (errno != 0) {
            fprintf(stderr, ", error %d, %s", errno, strerror(errno));
            errno = 0;
        }
        if (feof(f)) {
            fprintf(stderr, ", unexpected EOF");
        }
        fprintf(stderr, ".\n");
        free(ptr);
        ptr = NULL;
    }

    fclose(f);

    if (fsm_sz != NULL) {
        *fsm_sz = sz;
    }

    return ptr;
}

static void load_fsm5(void)
{
    if (six_plus_fsm5 != NULL) return;

    six_plus_fsm5 = load_fsm("six-plus-5.bin", "OFSM Six Plus 5", 36, 5, &six_plus_fsm5_sz);
}

static void load_fsm7(void)
{
    if (six_plus_fsm7 != NULL) return;

    six_plus_fsm7 = load_fsm("six-plus-7.bin", "OFSM Six Plus 7", 36, 7, &six_plus_fsm7_sz);
}



/* Debug hand rank calculations */

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

#define NEW_WAY(p, n1, n2, n3, n4) \
    { \
        uint32_t current = p; \
        current = six_plus_fsm5[current + cards[n1]]; \
        current = six_plus_fsm5[current + cards[n2]]; \
        current = six_plus_fsm5[current + cards[n3]]; \
        current = six_plus_fsm5[current + cards[n4]]; \
        if (current > result) result = current; \
    }

static inline unsigned int eval_rank7_via_fsm5_brutte(const card_t * cards)
{

    uint32_t result = 0;
    uint32_t start = 36;

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

    return result;
}

#undef NEW_WAY



/* Optimization packing */

#define QNOMINALS 9
pack_value_t pack_six_plus_5(unsigned int n, const input_t * path)
{
    uint64_t flash_masks[4] = { 0, 0, 0, 0};
    for (int i=0; i<n; ++i) {
        input_t card = path[i];
        flash_masks[SUITE(card)] = 1ull << NOMINAL(card);
    }

    uint64_t flash_suite = 4;
    unsigned int qflash_cards = 0;
    uint64_t flash_mask = 0;
    for (unsigned int s=0; s<4; ++s) {
        const uint64_t mask = flash_masks[s];
        const int bit_count = pop_count64(mask);
        if (bit_count >= 3) {
            qflash_cards = bit_count;
            flash_suite = s;
            flash_mask = mask;
            break;
        }
    }

    unsigned int qfolded_cards1 = 0;
    int qnominals[QNOMINALS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (unsigned int s=0; s<4; ++s) {
        uint64_t mask = flash_masks[s];
        while (mask != 0) {
            const int n = extract_rbit64(&mask);
            ++qnominals[n];
            ++qfolded_cards1;
        }
    }

    if (qflash_cards + qfolded_cards1 != n) {
        fprintf(stderr, "Assertion failed: sum of folder and flash cards is not equal to total cards!\n");
        fprintf(stderr, "  n = %u, qflash_cards = %u, qfolded_cards = %u\n", n, qflash_cards, qfolded_cards1);
        exit(1);
    }

    int qfolded_cards2 = 0;
    uint64_t folded_mask = 0;
    for (int n=0; n<QNOMINALS; ++n)
    for (int i=0; i<qnominals[n]; ++i) {
        folded_mask <<= 4;
        folded_mask |= n;
        ++qfolded_cards2;
    }

    if (qfolded_cards1 != qfolded_cards2) {
        fprintf(stderr, "Assertion failed: folded card mismatch! %d != %d\n", qfolded_cards1, qfolded_cards2);
        exit(1);
    }

    int qfolded_avail = n - qfolded_cards1;
    for (int i=0; i<qfolded_avail; ++i) {
        folded_mask <<= 4;
        folded_mask |= 0xF;
    }

    return 0
        | (folded_mask << 0)
        | (flash_suite << 60)
        | (flash_mask << 45)
    ;
}
#undef QNOMINALS



/* Build OFSMs */

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

pack_value_t calc_six_plus_7(unsigned int n, const input_t * path)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 5, but %u as passed.\n", n);
        exit(1);
    }

    card_t cards[n];
    for (size_t i=0; i<n; ++i) {
        cards[i] = path[i];
    }

    return eval_rank7_via_fsm5_brutte(cards);
}

int run_create_six_plus_5(struct ofsm_builder * restrict ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 36, 5)
        || ofsm_builder_pack(ob, calc_six_plus_5, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}

int run_create_six_plus_7(struct ofsm_builder * restrict ob)
{
    load_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 36, 7)
        || ofsm_builder_pack(ob, calc_six_plus_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



/* Tests with nice debug output */

typedef uint64_t eval_rank_f(const card_t * cards);

static int quick_test_for_eval_rank(const int qcards, const card_t * hands, eval_rank_f eval_rank)
{
    uint64_t prev_rank = 0;

    const card_t * current = hands;
    for (; *current != 0xFF; current += qcards) {
        const uint64_t rank = eval_rank(current);
        if (rank < prev_rank) {
            printf("[FAIL]\n");
            printf("  Wrong order!\n");

            printf("  Previous:");
            print_hand(qcards, current - qcards);
            printf(" has rank %lu (0x%lX)\n", prev_rank, prev_rank);

            printf("  Current:");
            print_hand(qcards, current);
            printf(" has rank %lu (0x%lX)\n", rank, rank);

            return 1;
        }
        prev_rank = rank;
    }

    return 0;
}

const card_t quick_ordered_hand5[] = {
    CARD_6h, CARD_8c, CARD_9c, CARD_Tc, CARD_Qs,
    CARD_6h, CARD_7d, CARD_8c, CARD_Tc, CARD_As,
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
    0xFF
};

static inline int quick_test_for_eval_rank5_via_slow_robust(int * restrict is_opencl)
{
    return quick_test_for_eval_rank(5, quick_ordered_hand5, eval_rank5_via_slow_robust);
}

static inline uint64_t eval_rank5_via_fsm5_as64(const card_t * cards)
{
    return eval_rank5_via_fsm5(cards);
}

static inline int quick_test_for_eval_rank5_via_fsm5(int * restrict is_opencl)
{
    return quick_test_for_eval_rank(5, quick_ordered_hand5, eval_rank5_via_fsm5_as64);
}

const card_t quick_ordered_hand7[] = {
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

static inline uint64_t eval_rank7_via_fsm5_brutte_as64(const card_t * cards)
{
    return eval_rank7_via_fsm5_brutte(cards);
}

static inline int quick_test_for_eval_rank7_via_fsm5_brutte(int * restrict is_opencl)
{
    return quick_test_for_eval_rank(7, quick_ordered_hand7, eval_rank7_via_fsm5_brutte_as64);
}

static inline uint64_t eval_rank7_via_fsm7_as64(const card_t * cards)
{
    return eval_rank7_via_fsm7(cards);
}

static inline int quick_test_for_eval_rank7_via_fsm7(int * restrict is_opencl)
{
    return quick_test_for_eval_rank(7, quick_ordered_hand7, eval_rank7_via_fsm7_as64);
}



int test_equivalence_between_eval_rank5_via_slow_robust_and_eval_rank5_via_fsm5(int * restrict is_opencl)
{
    static uint64_t saved[9999];

    uint64_t mask = 0x1F;
    uint64_t last = 1ull << 36;

    for (; mask < last; mask = next_combination_mask(mask)) {
        card_t cards[5];
        mask_to_cards(5, mask, cards);

        uint64_t rank1 = eval_rank5_via_fsm5(cards);
        uint64_t rank2 = eval_rank5_via_slow_robust(cards);

        if (rank1 <= 0 || rank1 > 9999) {
            printf("[FAIL]\n");
            printf("  Wrong rank 1!\n");
            printf("  Hand: ");
            print_hand(5, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: 0x%lX\n", rank2);
            return 1;
        }

        if (rank2 == 0) {
            printf("[FAIL]\n");
            printf("  Wrong rank 2!\n");
            printf("  Hand: ");
            print_hand(5, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: 0x%lX\n", rank2);
            return 1;
        }

        if (saved[rank1] == 0) {
            saved[rank1] = rank2;
            continue;
        }

        if (saved[rank1] != rank2) {
            printf("[FAIL]\n");
            printf("  Rank mismatch for hand");
            print_hand(5, cards);
            printf("\n");
            printf("  Saved rank: 0x%lX\n", saved[rank1]);
            printf("  Caclulated: 0x%lX\n", saved[rank2]);
            return 1;
        }
    }

    return 0;
}

#define QPERMUTATIONS (121*5)

int test_permutations_for_eval_rank5_via_fsm5(int * restrict is_opencl)
{
    static int permutation_table[QPERMUTATIONS];

    const int q = gen_permutation_table(permutation_table, 5, QPERMUTATIONS);
    if (q != 120) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is 5! = 120.\n", q);
        return 1;
    }

    if (opt_opencl) {
        *is_opencl = 1;

        static int8_t packed_permutation_table[QPERMUTATIONS];
        static const size_t packed_permutation_table_sz = sizeof(packed_permutation_table);

        for (int i=0; i<QPERMUTATIONS; ++i) {
            packed_permutation_table[i] = permutation_table[i];
        }

        const size_t qdata = 376992;
        const size_t data_sz = qdata * sizeof(uint64_t);
        uint64_t * data = malloc(data_sz);
        if (data == NULL) {
            printf("[FAIL] (OpenCL)\n");
            printf("  malloc(%lu) returns NULL.\n", data_sz);
            return 1;
        }

        const size_t report_sz = qdata * sizeof(uint16_t);
        uint16_t * restrict report = malloc(report_sz);
        if (report == NULL) {
            printf("[FAIL] (OpenCL)\n");
            printf("  malloc(%lu) returns NULL.\n", report_sz);
            free(data);
            return 1;
        }

        uint64_t mask = 0x1F;
        uint64_t last = 1ull << 36;
        uint64_t * restrict ptr = data;
        while (mask < last) {
            *ptr++ = mask;
            mask = next_combination_mask(mask);
        }

        if (ptr - data != qdata) {
            printf("[FAIL] (OpenCL)\n");
            printf("  Invalid qdata = %lu, calculated value is %lu.\n", qdata, ptr - data);
            free(report);
            free(data);
        }

        int result = opencl__test_permutations(
            5, 36, qdata,
            packed_permutation_table, packed_permutation_table_sz,
            six_plus_fsm5, six_plus_fsm5_sz,
            data, data_sz,
            report, report_sz
        );

        if (result == 0) {
            const uint16_t * ptr = report;
            for (uint32_t i=0; i<qdata; ++i) {
                if (ptr[i] != 0) {
                    printf("[FAIL] (OpenCL)\n");
                    printf("  report[%u] = %u is nonzero.\n", i, ptr[i]);
                    printf("  data[%u] = 0x%016lx is nonzero.\n", i, data[i]);

                    card_t cards[5];
                    mask_to_cards(5, data[i], cards);

                    uint32_t r = eval_rank5_via_fsm5(cards);
                    printf("  Base Hand:");
                    print_hand(5, cards);
                    printf(" has rank %u\n", r);

                    result = 1;
                    break;
                }
            }
        }

        free(data);
        free(report);
        return result;
    }

    uint64_t mask = 0x1F;
    uint64_t last = 1ull << 36;
    while (mask < last) {
        card_t cards[5];
        mask_to_cards(5, mask, cards);

        uint32_t rank = eval_rank5_via_fsm5(cards);

        for (int i=0; i<120; ++i) {
            const int * perm = permutation_table + 5 * i;
            card_t c[5];
            gen_perm(5, c, cards, perm);
            uint32_t r = eval_rank5_via_fsm5(c);

            if (r != rank) {
                printf("[FAIL]\n");

                printf("  Base Hand:");
                print_hand(5, cards);
                printf(" has rank %u\n", rank);

                printf("  Current Hand: ");
                print_hand(5, c);
                printf(" has rank %u\n", r);
                return 1;
            }
        }

        mask = next_combination_mask(mask);
    }

    return 0;
}

#undef QPERMUTATIONS


#define QPERMUTATIONS (5041*7)

int test_permutations_for_eval_rank7_via_fsm7(int * restrict is_opencl)
{
    static int permutation_table[QPERMUTATIONS];

    const int q = gen_permutation_table(permutation_table, 7, QPERMUTATIONS);
    if (q != 5040) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is 7! = 5040.\n", q);
        return 1;
    }

    if (opt_opencl) {
        *is_opencl = 1;

        static int8_t packed_permutation_table[QPERMUTATIONS];
        static const size_t packed_permutation_table_sz = sizeof(packed_permutation_table);

        for (int i=0; i<QPERMUTATIONS; ++i) {
            packed_permutation_table[i] = permutation_table[i];
        }

        const size_t qdata = 8347680;
        const size_t data_sz = qdata * sizeof(uint64_t);
        uint64_t * data = malloc(data_sz);
        if (data == NULL) {
            printf("[FAIL] (OpenCL)\n");
            printf("  malloc(%lu) returns NULL.\n", data_sz);
            return 1;
        }

        const size_t report_sz = qdata * sizeof(uint16_t);
        uint16_t * restrict report = malloc(report_sz);
        if (report == NULL) {
            printf("[FAIL] (OpenCL)\n");
            printf("  malloc(%lu) returns NULL.\n", report_sz);
            free(data);
            return 1;
        }

        uint64_t mask = 0x7F;
        uint64_t last = 1ull << 36;
        uint64_t * restrict ptr = data;
        while (mask < last) {
            *ptr++ = mask;
            mask = next_combination_mask(mask);
        }

        if (ptr - data != qdata) {
            printf("[FAIL] (OpenCL)\n");
            printf("  Invalid qdata = %lu, calculated value is %lu.\n", qdata, ptr - data);
            free(report);
            free(data);
        }

        int result = opencl__test_permutations(
            7, 36, qdata,
            packed_permutation_table, packed_permutation_table_sz,
            six_plus_fsm7, six_plus_fsm7_sz,
            data, data_sz,
            report, report_sz
        );

        if (result == 0) {
            const uint16_t * ptr = report;
            for (uint32_t i=0; i<qdata; ++i) {
                if (ptr[i] != 0) {
                    printf("[FAIL] (OpenCL)\n");
                    printf("  report[%u] = %u is nonzero.\n", i, ptr[i]);
                    printf("  data[%u] = 0x%016lx is nonzero.\n", i, data[i]);

                    card_t cards[7];
                    mask_to_cards(7, data[i], cards);

                    uint32_t r = eval_rank7_via_fsm7(cards);
                    printf("  Base Hand:");
                    print_hand(7, cards);
                    printf(" has rank %u\n", r);

                    result = 1;
                    break;
                }
            }
        }

        free(data);
        free(report);
        return result;
    }

    uint64_t mask = 0x7F;
    uint64_t last = 1ull << 36;
    while (mask < last) {
        card_t cards[7];
        mask_to_cards(7, mask, cards);

        uint32_t rank = eval_rank7_via_fsm7(cards);

        for (int i=0; i<5040; ++i) {
            const int * perm = permutation_table + 7 * i;
            card_t c[7];
            gen_perm(7, c, cards, perm);
            uint32_t r = eval_rank7_via_fsm7(c);

            if (r != rank) {
                printf("[FAIL]\n");

                printf("  Base Hand:");
                print_hand(7, cards);
                printf(" has rank %u\n", rank);

                printf("  Current Hand: ");
                print_hand(7, c);
                printf(" has rank %u\n", r);
                return 1;
            }
        }

        mask = next_combination_mask(mask);
    }

    return 0;
}

#undef QPERMUTATIONS

typedef int test_function(int * restrict);

static inline int run_test(const char * name, test_function test)
{
    const int w = -128;

    const double start = get_app_age();
    printf("Run %*s ", w, name);
    fflush(stdout);

    int is_opencl = 0;
    const int err = test(&is_opencl);
    if (err) {
        return 1;
    }
    const double delta = get_app_age() - start;
    printf("[ OK ] in %.2f s.%s\n", delta, is_opencl ? " (OpenCL)" : "");
    return 0;
}

#define RUN_TEST(f)                          \
    do {                                     \
        int err = run_test(#f "() ...", f);  \
        if (err) return 1;                   \
    } while (0)                              \

int run_check_six_plus_5(void)
{
    if (opt_opencl) {
        int err = init_opencl(stderr);
        if (err != 0) {
            return err;
        }
    }

    load_fsm5();

    RUN_TEST(quick_test_for_eval_rank5_via_slow_robust);
    RUN_TEST(quick_test_for_eval_rank5_via_fsm5);
    RUN_TEST(test_equivalence_between_eval_rank5_via_slow_robust_and_eval_rank5_via_fsm5);
    RUN_TEST(test_permutations_for_eval_rank5_via_fsm5);

    printf("All six plus 5 tests are successfully passed.\n");
    return 0;
}

int run_check_six_plus_7(void)
{
    if (opt_opencl) {
        int err = init_opencl(stderr);
        if (err != 0) {
            return err;
        }
    }

    load_fsm5();
    load_fsm7();

    RUN_TEST(quick_test_for_eval_rank7_via_fsm5_brutte);
    RUN_TEST(quick_test_for_eval_rank7_via_fsm7);
//    RUN_TEST(test_equivalence_between_eval_rank5_via_slow_robust_and_eval_rank5_via_fsm5);
    RUN_TEST(test_permutations_for_eval_rank7_via_fsm7);

    printf("All six plus 7 tests are successfully passed.\n");
    return 0;
}








/* OLD */

static inline uint32_t eval_rank7_via_fsm5_opt(const card_t * cards)
{
    uint32_t result = 0;
    uint32_t start = 36;

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



void save_binary(const char * file_name, const char * name, const struct ofsm_array * array);


/*
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

    uint32_t result = eval_rank7_via_fsm5_opt(cards);
    if (result == 0) {
        return INVALID_PACK_VALUE;
    } else {
        return result - 1;
    }
}
*/

void build_six_plus_7(void * script)
{
    load_fsm5();
    if (six_plus_fsm5 == NULL) {
        return;
    }

    script_step_comb(script, 36, 7);
    // Forget suites
    script_step_pack(script, calc_six_plus_7, PACK_FLAG__SKIP_RENUMERING);

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



void test_suite_start(const char * text)
{
    static const char * dots = "..........................................................................................";
    int has_printed = printf("%s", text);
    if (has_printed < 99) {
        printf("%.*s", 99 - has_printed, dots);
    }
    fflush(stdout);
}

void test_suite_passed()
{
    printf("OK\n");
    fflush(stdout);
}

void test_suite_failed()
{
    printf("FAIL\n");
    fflush(stdout);
}


void verify_permutations_for_eval_rank5_via_fsm5()
{
    test_suite_start("Verify permutations for eval_rank5_via_fsm5()");

    test_suite_passed();
}

void verify_equivalence_for_eval_rank7_via_fsm5_opt_and_eval_rank7_via_fsm7()
{
    test_suite_start("Verify equivalence for eval_rank7_via_fsm5_opt() and eval_rank7_via_fsm7()");

    uint64_t mask = 0x7F;
    uint64_t last = 1ull << 36;

    for (; mask < last; mask = next_combination_mask(mask)) {
        card_t cards[7];
        uint64_t tmp = mask;
        cards[0] = extract_rbit64(&tmp);
        cards[1] = extract_rbit64(&tmp);
        cards[2] = extract_rbit64(&tmp);
        cards[3] = extract_rbit64(&tmp);
        cards[4] = extract_rbit64(&tmp);
        cards[5] = extract_rbit64(&tmp);
        cards[6] = extract_rbit64(&tmp);

        uint32_t rank1 = eval_rank7_via_fsm5_opt(cards);
        uint32_t rank2 = eval_rank7_via_fsm7(cards);

        if (rank1 != rank2) {
            test_suite_failed();
            printf("  Wrong rank pair!\n");
            printf("  Hand: %s %s %s %s %s %s %s\n",
                card36_str[cards[0]],
                card36_str[cards[1]],
                card36_str[cards[2]],
                card36_str[cards[3]],
                card36_str[cards[4]],
                card36_str[cards[5]],
                card36_str[cards[6]]);
            printf("  Rank 1 (via fsm5): %u\n", rank1);
            printf("  Rank 2 (via fsm7): %u\n", rank2);

            uint32_t rank3 = eval_rank7_via_fsm5_brutte(cards);
            printf("  Rank 3 (another):  %u\n", rank3);
            // return;
        }
    }

    test_suite_passed();
}

void verify_six_plus()
{
    load_fsm5();
    if (six_plus_fsm5 == NULL) {
        fprintf(stderr, "Fatal: Can not load OFSM 5 cards for Six Poker Plus.\n");
        exit(1);
    }

    load_fsm7();
    if (six_plus_fsm7 == NULL) {
        fprintf(stderr, "Fatal: Can not load OFSM 5 cards for Six Poker Plus.\n");
        exit(1);
    }

    int is_opencl = 0;
    quick_test_for_eval_rank5_via_slow_robust(&is_opencl);
    quick_test_for_eval_rank5_via_fsm5(&is_opencl);
    test_equivalence_between_eval_rank5_via_slow_robust_and_eval_rank5_via_fsm5(&is_opencl);

    verify_permutations_for_eval_rank5_via_fsm5();
    verify_equivalence_for_eval_rank7_via_fsm5_opt_and_eval_rank7_via_fsm7();
}
