#include <errno.h>
#include <string.h>

#include "yoo-combinatoric.h"
#include "yoo-ofsm.h"
#include "yoo-stdlib.h"

#include "poker.h"

typedef uint64_t eval_rank_f(const card_t * cards);

struct test_desc
{
    int is_opencl;
    int qcards_in_hand;
    int qcards_in_deck;
    eval_rank_f * eval_rank;
    eval_rank_f * eval_rank_robust;
};



/* Our GOAL */

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

    return eval_rank5_via_slow_robust_for_deck36(cards);
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

static inline int quick_test_for_eval_rank5_via_slow_robust(struct test_desc * restrict me)
{
    return quick_test_for_eval_rank(5, quick_ordered_hand5_for_deck36, eval_rank5_via_slow_robust_for_deck36);
}

static inline uint64_t eval_rank5_via_fsm5_as64(const card_t * cards)
{
    return eval_rank5_via_fsm5(cards);
}

static inline int quick_test_for_eval_rank5_via_fsm5(struct test_desc * restrict me)
{
    return quick_test_for_eval_rank(5, quick_ordered_hand5_for_deck36, eval_rank5_via_fsm5_as64);
}

static inline uint64_t eval_rank7_via_fsm5_brutte_as64(const card_t * cards)
{
    return eval_rank7_via_fsm5_brutte(cards);
}

static inline int quick_test_for_eval_rank7_via_fsm5_brutte(struct test_desc * restrict me)
{
    return quick_test_for_eval_rank(7, quick_ordered_hand7_for_deck36, eval_rank7_via_fsm5_brutte_as64);
}

static inline uint64_t eval_rank7_via_fsm7_as64(const card_t * cards)
{
    return eval_rank7_via_fsm7(cards);
}

static inline int quick_test_for_eval_rank7_via_fsm7(struct test_desc * restrict me)
{
    return quick_test_for_eval_rank(7, quick_ordered_hand7_for_deck36, eval_rank7_via_fsm7_as64);
}



int test_equivalence(struct test_desc * restrict me)
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
            print_hand(me->qcards_in_hand, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }

        if (rank2 == 0) {
            printf("[FAIL]\n");
            printf("  Wrong rank 2!\n");
            printf("  Hand: ");
            print_hand(me->qcards_in_hand, cards);
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
            print_hand(me->qcards_in_hand, cards);
            printf("\n");
            printf("  Saved rank: %lu (0x%lX)\n", saved_rank, saved_rank);
            printf("  Caclulated: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }
    }

    return 0;
}

#define QPERMUTATIONS (121*5)

int test_permutations_for_eval_rank5_via_fsm5(struct test_desc * restrict me)
{
    static int permutation_table[QPERMUTATIONS];

    const int q = gen_permutation_table(permutation_table, 5, QPERMUTATIONS);
    if (q != 120) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is 5! = 120.\n", q);
        return 1;
    }

    if (opt_opencl) {
        me->is_opencl = 1;

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

int test_permutations_for_eval_rank7_via_fsm7(struct test_desc * restrict me)
{
    static int permutation_table[QPERMUTATIONS];

    const int q = gen_permutation_table(permutation_table, 7, QPERMUTATIONS);
    if (q != 5040) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is 7! = 5040.\n", q);
        return 1;
    }

    if (opt_opencl) {
        me->is_opencl = 1;

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

typedef int test_function(struct test_desc * restrict);

static inline int run_test(struct test_desc * restrict me, const char * name, test_function test)
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

int run_check_six_plus_5(void)
{
    printf("Six plus 5 tests:\n");

    struct test_desc desc = {
        .qcards_in_hand = 5,
        .qcards_in_deck = 36,
        .eval_rank = eval_rank5_via_fsm5_as64,
        .eval_rank_robust = eval_rank5_via_slow_robust_for_deck36
    };

    load_fsm5();

    RUN_TEST(&desc, quick_test_for_eval_rank5_via_slow_robust);
    RUN_TEST(&desc, quick_test_for_eval_rank5_via_fsm5);
    RUN_TEST(&desc, test_equivalence);
    RUN_TEST(&desc, test_permutations_for_eval_rank5_via_fsm5);

    printf("All six plus 5 tests are successfully passed.\n");
    return 0;
}

int run_check_six_plus_7(void)
{
    printf("Six plus 7 tests:\n");

    struct test_desc desc = {
        .qcards_in_hand = 7,
        .qcards_in_deck = 36,
        .eval_rank = eval_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_rank7_via_fsm5_brutte_as64
    };

    load_fsm5();
    load_fsm7();

    RUN_TEST(&desc, quick_test_for_eval_rank7_via_fsm5_brutte);
    RUN_TEST(&desc, quick_test_for_eval_rank7_via_fsm7);
    RUN_TEST(&desc, test_equivalence);
    RUN_TEST(&desc, test_permutations_for_eval_rank7_via_fsm7);

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

    // int is_opencl = 0;
    // quick_test_for_eval_rank5_via_slow_robust(&is_opencl);
    // quick_test_for_eval_rank5_via_fsm5(&is_opencl);
    // test_equivalence_between_eval_rank5_via_slow_robust_and_eval_rank5_via_fsm5(&is_opencl);

    verify_permutations_for_eval_rank5_via_fsm5();
    verify_equivalence_for_eval_rank7_via_fsm5_opt_and_eval_rank7_via_fsm7();
}
