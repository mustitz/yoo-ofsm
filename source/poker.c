#include <errno.h>
#include <string.h>

#include "yoo-combinatoric.h"
#include "yoo-ofsm.h"
#include "yoo-stdlib.h"

#include "poker.h"



/* Our GOAL */

const uint32_t * six_plus_fsm5;
const uint32_t * six_plus_fsm7;
const uint32_t * texas_fsm5;
const uint32_t * texas_fsm7;
const uint32_t * omaha_fsm7;

uint64_t six_plus_fsm5_sz;
uint64_t six_plus_fsm7_sz;
uint64_t texas_fsm5_sz;
uint64_t texas_fsm7_sz;
uint64_t omaha_fsm7_sz;

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

static inline uint32_t eval_omaha_rank7_via_fsm7(const card_t * cards)
{
    uint32_t current = 52;
    current = omaha_fsm7[current + cards[0]];
    current = omaha_fsm7[current + cards[1]];
    current = omaha_fsm7[current + cards[2]];
    current = omaha_fsm7[current + cards[3]];
    current = omaha_fsm7[current + cards[4]];
    current = omaha_fsm7[current + cards[5]];
    current = omaha_fsm7[current + cards[6]];
    return current;
}



/* Game data */

typedef uint64_t user_eval_rank_f(void * user_data, const card_t * cards);
typedef uint64_t eval_rank_robust_f(const card_t * cards);
typedef uint32_t eval_rank_f(const card_t * cards);

struct game_data
{
    const char * name;
    int qcards_in_deck;
    int qranks;
    int qhand_categories[9];
    int expected_stat5[9];
    const char * hand_category_names[9];
};

struct game_data six_plus_holdem = {
    .name = "Six Plus Hold`em",
    .qcards_in_deck = 36,
    .qranks = 1404,
    .qhand_categories = { 6, 72, 120, 72, 252, 6, 252, 504, 120 },
    .expected_stat5 = { 24, 288, 480, 1728, 16128, 6120, 36288, 193536, 122400 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Flush", "Full house", "Three of a kind", "Straight", "Two pair", "One pair", "High card" },
};

struct game_data texas_holdem = {
    .name = "Texas Hold`em",
    .qcards_in_deck = 52,
    .qranks = 7462,
    .qhand_categories = { 10, 156, 156, 1277, 10, 858, 858, 2860, 1277 },
    .expected_stat5 = { 40, 624, 3744, 5108, 10200, 54912, 123552, 1098240, 1302540 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Full house", "Flush", "Straight", "Three of a kind", "Two pair", "One pair", "High card" },
};

struct game_data omaha = {
    .name = "Omaha",
    .qcards_in_deck = 52,
    .qranks = 7462,
    .qhand_categories = { 10, 156, 156, 1277, 10, 858, 858, 2860, 1277 },
    .expected_stat5 = { -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Full house", "Flush", "Straight", "Three of a kind", "Two pair", "One pair", "High card" },
};

struct test_data
{
    const struct game_data * game;
    int is_opencl;
    int qcards_in_hand;
    void * user_data;
    int strict_equivalence;
    user_eval_rank_f * eval_rank;
    user_eval_rank_f * eval_rank_robust;
    const uint32_t * fsm;
    uint64_t fsm_sz;
    int * hand_type_stats;
};



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

static void load_texas_fsm7(void)
{
    if (texas_fsm7 != NULL) return;

    texas_fsm7 = load_fsm("texas-7.bin", "OFSM Texas 7", 52, 7, &texas_fsm7_sz);
}

static void load_omaha_fsm7(void)
{
    if (omaha_fsm7 != NULL) return;

    omaha_fsm7 = load_fsm("omaha-7.bin", "OFSM Omaha 7", 52, 7, &omaha_fsm7_sz);
}



/* Optimization packing */

pack_value_t forget_suites(unsigned int n, const input_t * path, unsigned int qmin)
{
    pack_value_t result = 0;

    unsigned int nominal_stat[13] = { 0 };
    unsigned int flush_mask[4] = { 0 };
    unsigned int flush_stat[4] = { 0 };

    for (int i=0; i<n; ++i) {
        const input_t card = path[i];
        const int suite = SUITE(card);
        const int nominal = NOMINAL(card);
        ++nominal_stat[nominal];
        flush_mask[suite] |= 1 << nominal;
        ++flush_stat[suite];
    }

    int suite_index = -1;
    unsigned int suite_mask = 0;
    for (int i=0; i<4; ++i) {
        if (flush_stat[i] < qmin) {
            continue;
        }

        if (suite_index != -1) {
            fprintf(stderr, "Assertion failed: we might remember more that one suite: %d and %d.\n", suite_index, i);
            abort();
        }

        suite_index = i;
        suite_mask = flush_mask[i];
    }

    if (suite_index >= 0) {
        result |= suite_index << 13;
        result |= suite_mask;
    }

    int shift = 16;
    for (int nominal=0; nominal<13; ++nominal) {
        for (unsigned int i=0; i<nominal_stat[nominal]; ++i) {
            if (shift > 60) {
                fprintf(stderr, "Assertion failed: shift overflow, value is %d, maximum is 60.\n", shift);
                abort();
            }
            pack_value_t tmp = nominal + 1;
            result |= tmp << shift;
            shift += 4;
        }
    }

    if (shift != 16 + 4*n) {
        fprintf(stderr, "Assertion failed: illegal shift value %d, expected %d.\n", shift, 16 + 4 * n);
    }

    return result;
}

static const int * get_perm_5_from_7(void)
{
    static const int * result = NULL;

    if (result != NULL) {
        return result;
    }

    size_t sz = 5 * 22 * sizeof(int);
    int * restrict ptr = global_malloc(sz);
    if (ptr == NULL) {
        fprintf(stderr, "Error: allocation error, malloc(%lu) failed with a NULL as a result.\n", sz);
        abort();
    }

    result = ptr;

    int qpermutations = 0;
    unsigned int mask = 0x1F;
    const unsigned int last = 1u << 7;
    while (mask < last) {
        unsigned int tmp = mask;
        ptr[0] = extract_rbit32(&tmp);
        ptr[1] = extract_rbit32(&tmp);
        ptr[2] = extract_rbit32(&tmp);
        ptr[3] = extract_rbit32(&tmp);
        ptr[4] = extract_rbit32(&tmp);
        ptr += 5;
        mask = next_combination_mask(mask);
        ++qpermutations;
    }

    ptr[0] = -1;
    ptr[1] = -1;
    ptr[2] = -1;
    ptr[3] = -1;
    ptr[4] = -1;

    if (qpermutations != 21) {
        fprintf(stderr, "Assertion failed, qpermutations = C(7,5) == %d != 21.\n", qpermutations);
        abort();
    }

    return result;
}

static const int * get_omaha_perm_5_from_7(void)
{
    static const int * result = NULL;
    if (result != NULL) {
        return result;
    }

    size_t sz = 5 * 11 * sizeof(int);
    int * restrict ptr = global_malloc(sz);
    if (ptr == NULL) {
        fprintf(stderr, "Error: allocation error, malloc(%lu) failed with a NULL as a result.\n", sz);
        abort();
    }

    result = ptr;

    int qpermutations = 0;
    unsigned int mask = 0x07;
    const unsigned int last = 1u << 5;
    while (mask < last) {
        unsigned int tmp = mask;
        ptr[0] = extract_rbit32(&tmp);
        ptr[1] = extract_rbit32(&tmp);
        ptr[2] = extract_rbit32(&tmp);
        ptr[3] = 5;
        ptr[4] = 6;
        ptr += 5;
        mask = next_combination_mask(mask);
        ++qpermutations;
    }

    ptr[0] = -1;
    ptr[1] = -1;
    ptr[2] = -1;
    ptr[3] = -1;
    ptr[4] = -1;

    if (qpermutations != 10) {
        fprintf(stderr, "Assertion failed, qpermutations = C(5,3) == %d != 10.\n", qpermutations);
        abort();
    }

    return result;
}

static inline void input_to_cards(unsigned int n, const input_t * const input, card_t * restrict const cards)
{
    for (size_t i=0; i<n; ++i) {
        cards[i] = input[i];
    }
}

static uint32_t eval_via_perm(eval_rank_f eval, const card_t * const cards, const int * perm)
{
    uint32_t rank = 0;
    card_t variant[5];

    for (;; perm += 5) {
        if (perm[0] == -1) {
            return rank;
        }

        for (int i=0; i<5; ++i) {
            variant[i] = cards[perm[i]];
        }

        const uint32_t tmp = eval(variant);
        if (tmp > rank) {
            rank = tmp;
        }
    }
}



/* Creating OFSMs */

pack_value_t calc_six_plus_5(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 5, but %u as passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_rank5_via_robust_for_deck36(cards);
}

int create_six_plus_5(struct ofsm_builder * restrict ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 36, 5)
        || ofsm_builder_pack(ob, calc_six_plus_5, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}



pack_value_t calc_six_plus_7(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 7, but %u was passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_six_plus_rank5_via_fsm5, cards, get_perm_5_from_7());
}

uint64_t calc_six_plus_7_flake_7_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 4);
}

uint64_t calc_six_plus_7_flake_6_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 3);
}

int create_six_plus_7(struct ofsm_builder * restrict ob)
{
    load_six_plus_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 36, 7)
        || ofsm_builder_pack(ob, calc_six_plus_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 1, calc_six_plus_7_flake_7_hash)
        || ofsm_builder_optimize(ob, 6, 1, calc_six_plus_7_flake_6_hash)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



pack_value_t calc_texas_5(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: %s requires n = 5, but %u as passed.\n", __FUNCTION__, n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_rank5_via_robust_for_deck52(cards);
}

uint64_t calc_texas_5_flake_5_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 4);
}

int create_texas_5(struct ofsm_builder * restrict ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 52, 5)
        || ofsm_builder_pack(ob, calc_texas_5, 0)
        || ofsm_builder_optimize(ob, 5, 1, calc_texas_5_flake_5_hash)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}



pack_value_t calc_texas_7(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 7, but %u was passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_perm_5_from_7());
}

uint64_t calc_texas_7_flake_7_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 4);
}

uint64_t calc_texas_7_flake_6_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 3);
}

int create_texas_7(struct ofsm_builder * restrict ob)
{
    load_texas_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 52, 7)
        || ofsm_builder_pack(ob, calc_texas_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 1, calc_texas_7_flake_7_hash)
        // || ofsm_builder_optimize(ob, 6, 1, calc_texas_7_flake_6_hash)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



pack_value_t calc_omaha_7_flake_5_pack(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: %s requires n = 5, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    return forget_suites(n, input, 3);
}

pack_value_t calc_omaha_7_flake_2_pack(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 2) {
        fprintf(stderr, "Assertion failed: %s requires n = 2, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    return forget_suites(n, input, 2);
}

pack_value_t calc_omaha_7(void * user_data, unsigned int n, const input_t * input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: %s requires n = 7, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_omaha_perm_5_from_7());
}

uint64_t calc_omaha_7_flake_7_hash(void * user_data, const unsigned int qjumps, const state_t * jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 4);
}

int create_omaha_7(struct ofsm_builder * restrict ob)
{
    load_texas_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 52, 5)
        || ofsm_builder_pack(ob, calc_omaha_7_flake_5_pack, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
        || ofsm_builder_push_comb(ob, 52, 2)
    //    || ofsm_builder_pack(ob, calc_omaha_7_flake_2_pack, 0)
    //    || ofsm_builder_optimize(ob, 2, 0, NULL)
        || ofsm_builder_product(ob)
        || ofsm_builder_pack(ob, calc_omaha_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 1, calc_omaha_7_flake_7_hash)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



/* Library */

const char * suite_str = "hdcs";

static inline void mask_to_cards(const int n, uint64_t mask, card_t * restrict cards)
{
    for (int i=0; i<n; ++i) {
        cards[i] = extract_rbit64(&mask);
    }
}

static inline void print_hand(const struct test_data * const me, const card_t * const cards)
{
    const char * const * card_str = NULL;
    switch (me->game->qcards_in_deck) {
        case 36:
            card_str = card36_str;
            break;
        case 52:
            card_str = card52_str;
            break;
        default:
            printf(" ??? unsupported qcards_in_deck = %d ???", me->game->qcards_in_deck);
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



/* Debug hand rank calculations */

static inline uint32_t eval_rank_via_fsm(const int qcards_in_hand, const card_t * cards, const uint32_t * const fsm, const uint32_t qcards_in_deck)
{
    uint32_t current = qcards_in_deck;
    for (int i=0; i<qcards_in_hand; ++i) {
        current = fsm[current + cards[i]];
    }
    return current;
}

static inline uint32_t test_data_eval_rank_via_fsm(const struct test_data * const me, const card_t * cards)
{
    return eval_rank_via_fsm(me->qcards_in_hand, cards, me->fsm, me->game->qcards_in_deck);
}

pack_value_t eval_rank5_via_robust_for_deck36_as64(void * user_data, const card_t * cards)
{
    return eval_rank5_via_robust_for_deck36(cards);
}

pack_value_t eval_rank5_via_robust_for_deck52_as64(void * user_data, const card_t * cards)
{
    return eval_rank5_via_robust_for_deck52(cards);
}



/* Quick tests */

static int quick_test_for_eval_rank(const struct test_data * const me, const int is_robust, const card_t * const hands)
{
    user_eval_rank_f * eval = is_robust ? me->eval_rank_robust : me->eval_rank;

    uint64_t prev_rank = 0;

    const card_t * current = hands;
    for (; *current != 0xFF; current += me->qcards_in_hand) {
        const uint64_t rank = eval(me->user_data, current);
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

static inline int quick_test_for_robust(struct test_data * restrict const me, const card_t * const hands)
{
    return quick_test_for_eval_rank(me, 1, hands);
}

static inline int quick_test(struct test_data * restrict const me, const card_t * const hands)
{
    return quick_test_for_eval_rank(me, 0, hands);
}



/* Equivalence test */

int test_equivalence(struct test_data * restrict const me)
{
    static uint64_t saved[9999];
    memset(saved, 0, sizeof(saved));

    uint64_t mask = (1ull << me->qcards_in_hand) - 1;
    uint64_t last = 1ull << me->game->qcards_in_deck;

    for (; mask < last; mask = next_combination_mask(mask)) {
        card_t cards[me->qcards_in_hand];
        mask_to_cards(me->qcards_in_hand, mask, cards);

        uint64_t rank1 = me->eval_rank(me->user_data, cards);
        uint64_t rank2 = me->eval_rank_robust(me->user_data, cards);

        if (rank1 <= 0 || rank1 > 9999) {
            printf("[FAIL]\n");
            printf("  Wrong rank (1)!\n");
            printf("  Hand: ");
            print_hand(me, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: %lu (0x%lX)\n", rank2, rank2);
            return 1;
        }

        if (rank2 == 0) {
            printf("[FAIL]\n");
            printf("  Wrong rank (2)!\n");
            printf("  Hand: ");
            print_hand(me, cards);
            printf("\n");
            printf("  Rank 1: %lu\n", rank1);
            printf("  Rank 2: %lu (0x%lX)\n", rank2, rank2);
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
        } else {
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
        }
    }

    return 0;
}



/* Permutation test */

int test_permutations(struct test_data * restrict const me)
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
    const uint64_t last = 1ull << me->game->qcards_in_deck;

    if (opt_opencl) {
        me->is_opencl = 1;

        int8_t packed_permutation_table[len];
        const size_t packed_permutation_table_sz = sizeof(packed_permutation_table);

        for (int i=0; i<len; ++i) {
            packed_permutation_table[i] = permutation_table[i];
        }

        const size_t qdata = calc_choose(me->game->qcards_in_deck, me->qcards_in_hand);
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
            me->qcards_in_hand, me->game->qcards_in_deck, qdata,
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
                    uint32_t r = test_data_eval_rank_via_fsm(me, cards);
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

        uint32_t rank = test_data_eval_rank_via_fsm(me, cards);

        for (int i=0; i<qpermutations; ++i) {
            const int * perm = permutation_table + me->qcards_in_hand * i;
            card_t c[me->qcards_in_hand];
            gen_perm(me->qcards_in_hand, c, cards, perm);
            uint32_t r = test_data_eval_rank_via_fsm(me, cards);

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



/* Stat test */

int test_stats(struct test_data * restrict const me)
{
    const int qranks = me->game->qranks;

    int stats[qranks+1];
    memset(stats, 0, sizeof(stats));

    const int * const qhand_categories = me->game->qhand_categories;

    int hand_category_low_ranges[9];
    hand_category_low_ranges[8] = 1;
    for (int i=7; i>=0; --i) {
        hand_category_low_ranges[i] = hand_category_low_ranges[i+1] + qhand_categories[i+1];
    }

    if (qhand_categories[0] + hand_category_low_ranges[0] != qranks + 1) {
        printf("[FAIL]\n");
        printf(
            "Error during hand_limit calculation: "
            "qhand_categories[0] + hand_category_low_ranges[0] != qranks + 1, "
            "%d + %d != %d + 1\n",
            qhand_categories[0], hand_category_low_ranges[0], qranks
        );
        return 1;
    }

    uint64_t mask = (1ull << me->qcards_in_hand) - 1;
    const uint64_t last = 1ull << me->game->qcards_in_deck;
    while (mask < last) {
        card_t cards[me->qcards_in_hand];
        mask_to_cards(me->qcards_in_hand, mask, cards);
        uint32_t rank = me->eval_rank(NULL, cards);
        if (rank < 0 || rank > qranks) {
            printf("[FAIL]\n");
            printf("Invalid rank = %u for hand:", rank);
            print_hand(me, cards);
            printf("\n");
            return 1;
        }

        ++stats[rank];
        mask = next_combination_mask(mask);
    }

    if (me->qcards_in_hand == 5) {
        for (int i=1; i<=qranks; ++i) {
            if (stats[i] == 0) {
                printf("[FAIL]\n");
                printf("Unexpected stats[%d] = 0.\n", i);
                return 1;
            }
        }
    }

    int * restrict hand_type_stats = me->hand_type_stats;

    for (int i=0; i<9; ++i) {
        hand_type_stats[i] = 0;
        int lo = hand_category_low_ranges[i];
        int hi = lo + qhand_categories[i];
        for (int j=lo; j<hi; ++j) {
            hand_type_stats[i] += stats[j];
        }
    }

    if (me->qcards_in_hand == 5) {
        for (int i=0; i<9; ++i) {
            if (hand_type_stats[i] != me->game->expected_stat5[i]) {
                printf("[FAIL]\n");
                printf("Invalid hand type stat: caclulated %d, expected %d for hand type %d.\n", hand_type_stats[i], me->game->expected_stat5[i], i);
                return 1;
            }
        }
    }

    return 0;
}

void print_stats(struct test_data * restrict const me)
{
    size_t total = 0;
    for (int i=0; i<9; ++i) {
        total += me->hand_type_stats[i];
    }

    if (total > 0) {
        printf("  Stats:\n");
        for (int i=0; i<9; ++i) {
            printf("    %*s %8d  %4.1f%%\n",
                -16, me->game->hand_category_names[i],
                me->hand_type_stats[i], 100.0 * me->hand_type_stats[i] / total
            );
        }
        printf("   ----------------------------------\n");
        printf("    Total            %8lu 100.0%%\n", total);
    }
}



/* Test environment */

typedef int test_function(struct test_data * restrict const);

static inline int run_test(struct test_data * restrict const me, const char * name, test_function test)
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



/* Six Plus fsm5 tests */

static int quick_test_six_plus_eval_rank5_robust(struct test_data * const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck36);
}

static int quick_test_six_plus_eval_rank5(struct test_data * const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck36);
}

static uint64_t eval_six_plus_rank5_via_fsm5_as64(void * user_data, const card_t * cards)
{
    return eval_six_plus_rank5_via_fsm5(cards);
}

int check_six_plus_5(void)
{
    printf("Six plus 5 tests:\n");

    load_six_plus_fsm5();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &six_plus_holdem,
        .qcards_in_hand = 5,
        .strict_equivalence = 0,
        .eval_rank = eval_six_plus_rank5_via_fsm5_as64,
        .eval_rank_robust = eval_rank5_via_robust_for_deck36_as64,
        .fsm = six_plus_fsm5,
        .fsm_sz = six_plus_fsm5_sz,
        .hand_type_stats = hand_type_stats,
    };

    RUN_TEST(&suite, quick_test_six_plus_eval_rank5_robust);
    RUN_TEST(&suite, quick_test_six_plus_eval_rank5);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);
    RUN_TEST(&suite, test_stats);

    print_stats(&suite);
    printf("  All six plus 5 tests are successfully passed.\n");
    return 0;
}



/* Six Plus fsm7 tests */

static uint64_t eval_six_plus_rank7_via_fsm7_as64(void * user_data, const card_t * cards)
{
    return eval_six_plus_rank7_via_fsm7(cards);
}

static uint64_t eval_six_plus_rank7_via_fsm5_brutte_as64(void * user_data, const card_t * cards)
{
    return eval_via_perm(eval_six_plus_rank5_via_fsm5, cards, get_perm_5_from_7());
}

static int quick_test_six_plus_eval_rank7_robust(struct test_data * const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand7_for_deck36);
}

static int quick_test_six_plus_eval_rank7(struct test_data * const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand7_for_deck36);
}

int check_six_plus_7(void)
{
    printf("Six plus 7 tests:\n");

    load_six_plus_fsm5();
    load_six_plus_fsm7();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &six_plus_holdem,
        .qcards_in_hand = 7,
        .user_data = NULL,
        .strict_equivalence = 1,
        .eval_rank = eval_six_plus_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_six_plus_rank7_via_fsm5_brutte_as64,
        .fsm = six_plus_fsm7,
        .fsm_sz = six_plus_fsm7_sz,
        .hand_type_stats = hand_type_stats
    };

    RUN_TEST(&suite, quick_test_six_plus_eval_rank7_robust);
    RUN_TEST(&suite, quick_test_six_plus_eval_rank7);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);
    RUN_TEST(&suite, test_stats);

    print_stats(&suite);
    printf("All six plus 7 tests are successfully passed.\n");
    return 0;
}



/* Texas fsm5 tests */

static uint64_t eval_texas_rank5_via_fsm5_as64(void * user_data, const card_t * cards)
{
    return eval_texas_rank5_via_fsm5(cards);
}

static int quick_test_texas_eval_rank5_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck52);
}

static int quick_test_texas_eval_rank5(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck52);
}

int check_texas_5(void)
{
    printf("Texas 5 tests:\n");

    load_texas_fsm5();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &texas_holdem,
        .qcards_in_hand = 5,
        .strict_equivalence = 0,
        .eval_rank = eval_texas_rank5_via_fsm5_as64,
        .eval_rank_robust = eval_rank5_via_robust_for_deck52_as64,
        .fsm = texas_fsm5,
        .fsm_sz = texas_fsm5_sz,
        .hand_type_stats = hand_type_stats
    };

    RUN_TEST(&suite, quick_test_texas_eval_rank5_robust);
    RUN_TEST(&suite, quick_test_texas_eval_rank5);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);
    RUN_TEST(&suite, test_stats);

    print_stats(&suite);
    printf("All texas 5 tests are successfully passed.\n");
    return 0;
}



/* Texas fsm7 tests */

static int quick_test_texas_eval_rank7_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand7_for_deck52);
}

static int quick_test_texas_eval_rank7(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand7_for_deck52);
}

static uint64_t eval_texas_rank7_via_fsm7_as64(void * user_data, const card_t * cards)
{
    return eval_texas_rank7_via_fsm7(cards);
}

static uint64_t eval_texas_rank7_via_fsm5_brutte_as64(void * user_data, const card_t * cards)
{
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_perm_5_from_7());
}

int check_texas_7(void)
{
    printf("Six plus 7 tests:\n");

    load_texas_fsm5();
    load_texas_fsm7();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &texas_holdem,
        .qcards_in_hand = 7,
        .user_data = NULL,
        .strict_equivalence = 1,
        .eval_rank = eval_texas_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_texas_rank7_via_fsm5_brutte_as64,
        .fsm = texas_fsm7,
        .fsm_sz = texas_fsm7_sz,
        .hand_type_stats = hand_type_stats
    };

    RUN_TEST(&suite, quick_test_texas_eval_rank7_robust);
    RUN_TEST(&suite, quick_test_texas_eval_rank7);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);
    RUN_TEST(&suite, test_stats);

    print_stats(&suite);
    printf("All texas 7 tests are successfully passed.\n");

    return 0;
}



/* Omaha fsm7 tests */

static uint64_t eval_omaha_rank7_via_fsm7_as64(void * user_data, const card_t * cards)
{
    return eval_omaha_rank7_via_fsm7(cards);
}

static uint64_t eval_omaha_rank7_via_fsm5_brutte_as64(void * user_data, const card_t * cards)
{
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_omaha_perm_5_from_7());
}

int check_omaha_7(void)
{
    printf("Omaha 7 tests:\n");

    load_texas_fsm5();
    load_omaha_fsm7();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &omaha,
        .qcards_in_hand = 7,
        .user_data = NULL,
        .strict_equivalence = 1,
        .eval_rank = eval_omaha_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_omaha_rank7_via_fsm5_brutte_as64,
        .fsm = omaha_fsm7,
        .fsm_sz = omaha_fsm7_sz,
        .hand_type_stats = hand_type_stats
    };

    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);

    printf("All omaha 7 tests are successfully passed.\n");
    return 0;
}



/* Debug code */

int debug_forget_suites(void)
{
    input_t input[2];

    uint64_t mask = 3;
    const uint64_t last = 1ull << 52;
    while (mask < last) {
        uint64_t tmp = mask;
        input[0] = extract_rbit64(&tmp);
        input[1] = extract_rbit64(&tmp);
        uint64_t test = forget_suites(2, input, 2);
        unsigned int suite_mask = test & 0xFFFF;
        unsigned int suite_index = suite_mask >> 13;
        suite_mask &= 0x1FFF;

        printf("%s %s - 0x%016lx %u 0x%04x\n", card52_str[input[0]], card52_str[input[1]], test >> 16, suite_index, suite_mask);
        mask = next_combination_mask(mask);
    }
    return 1;
}

uint32_t debug_calc_omaha_7(const card_t * const cards)
{
    uint32_t current = 52;
    current = omaha_fsm7[current + cards[0]];
    current = omaha_fsm7[current + cards[1]];
    current = omaha_fsm7[current + cards[2]];
    current = omaha_fsm7[current + cards[3]];
    current = omaha_fsm7[current + cards[4]];
    current = omaha_fsm7[current + cards[5]];
    current = omaha_fsm7[current + cards[6]];
    return current;
}

uint32_t debug_calc_omaha_5(const card_t * const cards)
{
    uint32_t current = 52;
    current = omaha_fsm7[current + cards[0]];
    current = omaha_fsm7[current + cards[1]];
    current = omaha_fsm7[current + cards[2]];
    current = omaha_fsm7[current + cards[3]];
    current = omaha_fsm7[current + cards[4]];
    return current;
}

uint32_t debug_eval_via_perm(eval_rank_f eval, const card_t * const cards, const int * perm)
{
    uint32_t rank = 0;
    card_t variant[5];

    for (;; perm += 5) {
        if (perm[0] == -1) {
            return rank;
        }

        for (int i=0; i<5; ++i) {
            variant[i] = cards[perm[i]];
        }

        const uint32_t tmp = eval(variant);
        if (tmp > rank) {
            rank = tmp;
        }

        printf("Hand:");
        for (int i=0; i<5; ++i) {
            printf(" %s", card52_str[variant[i]]);
        }
        printf(" - %4u\n", tmp);

    }
}

int debug_omaha_7(void)
{
    load_texas_fsm5();
    load_omaha_fsm7();

    const size_t qpermutations = factorial(5);
    const size_t len = 5 * (qpermutations + 1);
    int permutation_table[len];

    const int q = gen_permutation_table(permutation_table, 5, len);
    if (q != qpermutations) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count %d, expected value is %d! = %lu.\n", q, q, qpermutations);
        return 1;
    }

    card_t cards[7];
    cards[4] = 0 * 4 + SUITE_H;
    cards[3] = 1 * 4 + SUITE_H;
    cards[2] = 2 * 4 + SUITE_D;
    cards[1] = 3 * 4 + SUITE_D;
    cards[0] = 4 * 4 + SUITE_D;
    cards[5] = 4 * 4 + SUITE_D;
    cards[6] = 4 * 4 + SUITE_H;

    printf("Hand:");
    for (int i=0; i<7; ++i) {
        printf(" %s", card52_str[cards[i]]);
    }
    printf("\n");

    uint32_t state_5 = debug_calc_omaha_5(cards);
    printf("State 5 = %u\n", state_5);

    uint64_t mask = 0x1F;
    const uint64_t last = 1ull << 52;
    int counter = 0;
    while (mask < last) {
        card_t tmp[5];
        mask_to_cards(5, mask, tmp);
        uint32_t current_state_5 = debug_calc_omaha_5(tmp);
        if (current_state_5 == state_5) {
            input_t input[5] = { tmp[0], tmp[1], tmp[2], tmp[3], tmp[4] };
            pack_value_t pack_value = calc_omaha_7_flake_5_pack(NULL, 5, input);

            printf("  S5 Hand:");
            for (int i=0; i<5; ++i) {
                printf(" %s", card52_str[tmp[i]]);
            }
            printf(" %016lX (%d)\n", pack_value, ++counter);

            for (int i=0; i<qpermutations; ++i) {
                const int * perm = permutation_table + 5*i;
                card_t c[5];
                gen_perm(5, c, tmp, perm);
                input_t inp[5] = { c[0], c[1], c[2], c[3], c[4] };
                pack_value_t perm_pack_value = calc_omaha_7_flake_5_pack(NULL, 5, inp);
                if (perm_pack_value != pack_value) {
                    printf("    Perm fail:");
                    for (int i=0; i<5; ++i) {
                        printf(" %s", card52_str[inp[i]]);
                    }
                    printf(" %016lX\n", perm_pack_value);
                }
            }

        }
        mask = next_combination_mask(mask);
    }

    printf("Rank %u\n", debug_calc_omaha_7(cards));

    uint32_t debug_rank = debug_eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_omaha_perm_5_from_7());
    printf("Debug %u\n\n", debug_rank);
    return 1;
}

int debug_something(void)
{
    return 0;
}
