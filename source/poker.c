#include <errno.h>
#include <string.h>

#include "yoo-combinatoric.h"
#include "yoo-ofsm.h"
#include "yoo-stdlib.h"

#include "poker.h"

const char * suite_str = "hdcs";

typedef uint64_t eval_rank_f(const card_t * cards);

struct test_suite
{
    int is_opencl;
    int qcards_in_hand;
    int qcards_in_deck;
    int strict_equivalence;
    eval_rank_f * eval_rank;
    eval_rank_f * eval_rank_robust;
    const uint32_t * fsm;
    uint64_t fsm_sz;
};



/* Our GOAL */

const uint32_t * six_plus_fsm5;
const uint32_t * six_plus_fsm7;
const uint32_t * texas_fsm5;
const uint32_t * texas_fsm7;

uint64_t six_plus_fsm5_sz;
uint64_t six_plus_fsm7_sz;
uint64_t texas_fsm5_sz;
uint64_t texas_fsm7_sz;

static inline uint32_t eval_rank_via_fms(const int qcards_in_hand, const card_t * cards, const uint32_t * const fsm, const uint32_t qcards_in_deck)
{
    uint32_t current = qcards_in_deck;
    for (int i=0; i<qcards_in_hand; ++i) {
        current = fsm[current + cards[i]];
    }
    return current;
}

static inline uint32_t test_eval_rank_via_fms(const struct test_suite * const me, const card_t * cards)
{
    return eval_rank_via_fms(me->qcards_in_hand, cards, me->fsm, me->qcards_in_deck);
}

static inline uint32_t eval_six_plus_rank5_via_fsm5(const card_t * cards)
{
    uint32_t current = 36;
    current = six_plus_fsm5[current + cards[0]];
    current = six_plus_fsm5[current + cards[1]];
    current = six_plus_fsm5[current + cards[2]];
    current = six_plus_fsm5[current + cards[3]];
    current = six_plus_fsm5[current + cards[4]];
    return current;
}

static inline uint32_t eval_six_plus_rank7_via_fsm7(const card_t * cards)
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

static inline uint32_t eval_texas_rank5_via_fsm5(const card_t * cards)
{
    uint32_t current = 52;
    current = texas_fsm5[current + cards[0]];
    current = texas_fsm5[current + cards[1]];
    current = texas_fsm5[current + cards[2]];
    current = texas_fsm5[current + cards[3]];
    current = texas_fsm5[current + cards[4]];
    return current;
}

static inline uint32_t eval_texas_rank7_via_fsm7(const card_t * cards)
{
    uint32_t current = 52;
    current = texas_fsm7[current + cards[0]];
    current = texas_fsm7[current + cards[1]];
    current = texas_fsm7[current + cards[2]];
    current = texas_fsm7[current + cards[3]];
    current = texas_fsm7[current + cards[4]];
    current = texas_fsm7[current + cards[5]];
    current = texas_fsm7[current + cards[6]];
    return current;
}



/* Library */

static inline void mask_to_cards(const int n, uint64_t mask, card_t * restrict cards)
{
    for (int i=0; i<n; ++i) {
        cards[i] = extract_rbit64(&mask);
    }
}

static inline void print_hand(const struct test_suite * const me, const card_t * const cards)
{
    const char * const * card_str = NULL;
    switch (me->qcards_in_deck) {
        case 36:
            card_str = card36_str;
            break;
        case 52:
            card_str = card52_str;
            break;
        default:
            printf(" ??? unsupported qcards_in_deck = %d ???", me->qcards_in_deck);
            return;
    }

    for (int i=0; i<me->qcards_in_hand; ++i) {
        printf(" %s", card_str[cards[i]]);
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
    void * ptr = global_malloc(sz);
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

static void load_six_plus_fsm5(void)
{
    if (six_plus_fsm5 != NULL) return;

    six_plus_fsm5 = load_fsm("six-plus-5.bin", "OFSM Six Plus 5", 36, 5, &six_plus_fsm5_sz);
}

static void load_six_plus_fsm7(void)
{
    if (six_plus_fsm7 != NULL) return;

    six_plus_fsm7 = load_fsm("six-plus-7.bin", "OFSM Six Plus 7", 36, 7, &six_plus_fsm7_sz);
}

static void load_texas_fsm5(void)
{
    if (texas_fsm5 != NULL) return;

    texas_fsm5 = load_fsm("texas-5.bin", "OFSM Texas 5", 52, 5, &texas_fsm5_sz);
}



/* Debug hand rank calculations */

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

    return eval_rank5_via_robust_for_deck36(cards);
}

int run_create_six_plus_5(struct ofsm_builder * restrict ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 36, 5)
        || ofsm_builder_pack(ob, calc_six_plus_5, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
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

int run_create_six_plus_7(struct ofsm_builder * restrict ob)
{
    load_six_plus_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 36, 7)
        || ofsm_builder_pack(ob, calc_six_plus_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}

pack_value_t calc_texas_5(unsigned int n, const input_t * path)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: %s requires n = 5, but %u as passed.\n", __FUNCTION__, n);
        exit(1);
    }

    card_t cards[n];
    for (size_t i=0; i<n; ++i) {
        cards[i] = path[i];
    }

    return eval_rank5_via_robust_for_deck52(cards);
}

int run_create_texas_5(struct ofsm_builder * restrict ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 52, 5)
        || ofsm_builder_pack(ob, calc_texas_5, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}



/* Tests with nice debug output */

static int quick_test_for_eval_rank(const struct test_suite * const me, const int is_robust, const card_t * const hands)
{
    eval_rank_f * eval = is_robust ? me->eval_rank_robust : me->eval_rank;

    uint64_t prev_rank = 0;

    const card_t * current = hands;
    for (; *current != 0xFF; current += me->qcards_in_hand) {
        const uint64_t rank = eval(current);
        if (rank < prev_rank) {
            printf("[FAIL]\n");
            printf("  Wrong order!\n");

            printf("  Previous:");
            print_hand(me, current - me->qcards_in_hand);
            printf(" has rank %lu (0x%lX)\n", prev_rank, prev_rank);

            printf("  Current:");
            print_hand(me, current);
            printf(" has rank %lu (0x%lX)\n", rank, rank);

            return 1;
        }
        prev_rank = rank;
    }

    return 0;
}

static inline int quick_test_for_robust(struct test_suite * restrict const me, const card_t * const hands)
{
    return quick_test_for_eval_rank(me, 1, hands);
}

static inline int quick_test(struct test_suite * restrict const me, const card_t * const hands)
{
    return quick_test_for_eval_rank(me, 0, hands);
}



int test_equivalence(struct test_suite * restrict const me)
{
    static uint64_t saved[9999];
    memset(saved, 0, sizeof(saved));

    uint64_t mask = (1ull << me->qcards_in_hand) - 1;
    uint64_t last = 1ull << me->qcards_in_deck;

    for (; mask < last; mask = next_combination_mask(mask)) {
        card_t cards[me->qcards_in_hand];
        mask_to_cards(me->qcards_in_hand, mask, cards);

        uint64_t rank1 = me->eval_rank(cards);
        uint64_t rank2 = me->eval_rank_robust(cards);

        if (rank1 <= 0 || rank1 > 9999) {
            printf("[FAIL]\n");
            printf("  Wrong rank 1!\n");
            printf("  Hand: ");
            print_hand(me, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }

        if (rank2 == 0) {
            printf("[FAIL]\n");
            printf("  Wrong rank 2!\n");
            printf("  Hand: ");
            print_hand(me, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }

        if (saved[rank1] == 0) {
            saved[rank1] = rank2;
            continue;
        }

        uint64_t saved_rank = saved[rank1];
        if (saved_rank != rank2) {
            printf("[FAIL]\n");
            printf("  Rank mismatch for hand");
            print_hand(me, cards);
            printf("\n");
            printf("  Saved rank: %lu (0x%lX)\n", saved_rank, saved_rank);
            printf("  Caclulated: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }

        if (me->strict_equivalence) {
            if (rank1 != rank2) {
                printf("[FAIL]\n");
                printf("  Strict equivalence rank mismatch for hand");
                print_hand(me, cards);
                printf("\n");
                printf("  Actual rank: %lu (0x%lX)\n", rank1, rank1);
                printf("  Robust rank: %lu (0x%lX)\n", rank2, rank2);
                return 1;
            }
        }
    }

    return 0;
}

int test_permutations(struct test_suite * restrict const me)
{
    const size_t qpermutations = factorial(me->qcards_in_hand);
    const size_t len = me->qcards_in_hand * (qpermutations + 1);
    int permutation_table[len];

    const int q = gen_permutation_table(permutation_table, me->qcards_in_hand, len);
    if (q != qpermutations) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is %d! = %lu.\n", q, q, qpermutations);
        return 1;
    }

    uint64_t mask = (1ull << me->qcards_in_hand) - 1;
    const uint64_t last = 1ull << me->qcards_in_deck;

    if (opt_opencl) {
        me->is_opencl = 1;

        int8_t packed_permutation_table[len];
        const size_t packed_permutation_table_sz = sizeof(packed_permutation_table);

        for (int i=0; i<len; ++i) {
            packed_permutation_table[i] = permutation_table[i];
        }

        const size_t qdata = calc_choose(me->qcards_in_deck, me->qcards_in_hand);
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
            me->qcards_in_hand, me->qcards_in_deck, qdata,
            packed_permutation_table, packed_permutation_table_sz,
            me->fsm, me->fsm_sz,
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

                    card_t cards[me->qcards_in_hand];
                    mask_to_cards(me->qcards_in_hand, data[i], cards);
                    uint32_t r = test_eval_rank_via_fms(me, cards);
                    printf("  Base Hand:");
                    print_hand(me, cards);
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

    while (mask < last) {
        card_t cards[me->qcards_in_hand];
        mask_to_cards(me->qcards_in_hand, mask, cards);

        uint32_t rank = test_eval_rank_via_fms(me, cards);

        for (int i=0; i<qpermutations; ++i) {
            const int * perm = permutation_table + me->qcards_in_hand * i;
            card_t c[me->qcards_in_hand];
            gen_perm(me->qcards_in_hand, c, cards, perm);
            uint32_t r = test_eval_rank_via_fms(me, cards);

            if (r != rank) {
                printf("[FAIL]\n");

                printf("  Base Hand:");
                print_hand(me, cards);
                printf(" has rank %u\n", rank);

                printf("  Current Hand: ");
                print_hand(me, c);
                printf(" has rank %u\n", r);
                return 1;
            }
        }

        mask = next_combination_mask(mask);
    }

    return 0;
}



typedef int test_function(struct test_suite * restrict const);

static inline int run_test(struct test_suite * restrict const me, const char * name, test_function test)
{
    const int w = -128;

    const double start = get_app_age();
    printf("  Run %*s ", w, name);
    fflush(stdout);

    me->is_opencl = 0;
    const int err = test(me);
    if (err) {
        return 1;
    }
    const double delta = get_app_age() - start;
    printf("[ OK ] in %.2f s.%s\n", delta, me->is_opencl ? " (OpenCL)" : "");
    return 0;
}

#define RUN_TEST(desc, f)                          \
    do {                                           \
        int err = run_test(desc, #f "() ...", f);  \
        if (err) return 1;                         \
    } while (0)                                    \



static uint64_t eval_six_plus_rank5_via_fsm5_as64(const card_t * cards)
{
    return eval_six_plus_rank5_via_fsm5(cards);
}

static int quick_test_six_plus_eval_rank5_robust(struct test_suite * const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck36);
}

static int quick_test_six_plus_eval_rank5(struct test_suite * const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck36);
}

int run_check_six_plus_5(void)
{
    printf("Six plus 5 tests:\n");

    load_six_plus_fsm5();

    struct test_suite suite = {
        .qcards_in_hand = 5,
        .qcards_in_deck = 36,
        .strict_equivalence = 0,
        .eval_rank = eval_six_plus_rank5_via_fsm5_as64,
        .eval_rank_robust = eval_rank5_via_robust_for_deck36,
        .fsm = six_plus_fsm5,
        .fsm_sz = six_plus_fsm5_sz
    };

    RUN_TEST(&suite, quick_test_six_plus_eval_rank5_robust);
    RUN_TEST(&suite, quick_test_six_plus_eval_rank5);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);

    printf("All six plus 5 tests are successfully passed.\n");
    return 0;
}



static uint64_t eval_rank7_via_fsm7_as64(const card_t * cards)
{
    return eval_six_plus_rank7_via_fsm7(cards);
}

static uint64_t eval_rank7_via_fsm5_brutte_as64(const card_t * cards)
{
    return eval_rank7_via_fsm5_brutte(cards);
}

static int quick_test_six_plus_eval_rank7_robust(struct test_suite * const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand7_for_deck36);
}

static int quick_test_six_plus_eval_rank7(struct test_suite * const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand7_for_deck36);
}

int run_check_six_plus_7(void)
{
    printf("Six plus 7 tests:\n");

    load_six_plus_fsm5();
    load_six_plus_fsm7();

    struct test_suite suite = {
        .qcards_in_hand = 7,
        .qcards_in_deck = 36,
        .strict_equivalence = 1,
        .eval_rank = eval_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_rank7_via_fsm5_brutte_as64,
        .fsm = six_plus_fsm7,
        .fsm_sz = six_plus_fsm7_sz
    };

    RUN_TEST(&suite, quick_test_six_plus_eval_rank7_robust);
    RUN_TEST(&suite, quick_test_six_plus_eval_rank7);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);

    printf("All six plus 7 tests are successfully passed.\n");
    return 0;
}



static uint64_t eval_texas_rank5_via_fsm5_as64(const card_t * cards)
{
    return eval_texas_rank5_via_fsm5(cards);
}

static int quick_test_texas_eval_rank5_robust(struct test_suite * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck52);
}

static int quick_test_texas_eval_rank5(struct test_suite * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck52);
}

int run_check_texas_5(void)
{
    printf("Texas 5 tests:\n");

    load_texas_fsm5();

    struct test_suite suite = {
        .qcards_in_hand = 5,
        .qcards_in_deck = 52,
        .strict_equivalence = 0,
        .eval_rank = eval_texas_rank5_via_fsm5_as64,
        .eval_rank_robust = eval_rank5_via_robust_for_deck52,
        .fsm = texas_fsm5,
        .fsm_sz = texas_fsm5_sz
    };

    RUN_TEST(&suite, quick_test_texas_eval_rank5_robust);
    RUN_TEST(&suite, quick_test_texas_eval_rank5);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);

    printf("All six plus 5 tests are successfully passed.\n");
    return 0;
}