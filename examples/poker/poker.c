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

static inline uint32_t eval_six_plus_rank5_via_fsm5(const card_t * const cards)
{
    uint32_t current = 36;
    current = six_plus_fsm5[current + cards[0]];
    current = six_plus_fsm5[current + cards[1]];
    current = six_plus_fsm5[current + cards[2]];
    current = six_plus_fsm5[current + cards[3]];
    current = six_plus_fsm5[current + cards[4]];
    return current;
}

static inline uint32_t eval_six_plus_rank7_via_fsm7(const card_t * const cards)
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

static inline uint32_t eval_texas_rank5_via_fsm5(const card_t * const cards)
{
    uint32_t current = 52;
    current = texas_fsm5[current + cards[0]];
    current = texas_fsm5[current + cards[1]];
    current = texas_fsm5[current + cards[2]];
    current = texas_fsm5[current + cards[3]];
    current = texas_fsm5[current + cards[4]];
    return current;
}

static inline uint32_t eval_texas_rank7_via_fsm7(const card_t * const cards)
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

static inline uint32_t eval_omaha_rank7_via_fsm7(const card_t * const cards)
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

typedef uint64_t user_eval_rank_f(const card_t * cards);
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

struct test_data
{
    const struct game_data * game;
    int is_opencl;
    size_t counter;
    int qcards_in_hand1;
    int qcards_in_hand2;
    int strict_equivalence;
    user_eval_rank_f * eval_rank;
    user_eval_rank_f * eval_rank_robust;
    const uint32_t * fsm;
    uint64_t fsm_sz;
    uint64_t enumeration[5];
    int * hand_type_stats;
};



const struct game_data six_plus_holdem = {
    .name = "Six Plus Hold`em",
    .qcards_in_deck = 36,
    .qranks = 1404,
    .qhand_categories = { 6, 72, 120, 72, 252, 6, 252, 504, 120 },
    .expected_stat5 = { 24, 288, 480, 1728, 16128, 6120, 36288, 193536, 122400 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Flush", "Full house", "Three of a kind", "Straight", "Two pair", "One pair", "High card" },
};

const struct game_data texas_holdem = {
    .name = "Texas Hold`em",
    .qcards_in_deck = 52,
    .qranks = 7462,
    .qhand_categories = { 10, 156, 156, 1277, 10, 858, 858, 2860, 1277 },
    .expected_stat5 = { 40, 624, 3744, 5108, 10200, 54912, 123552, 1098240, 1302540 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Full house", "Flush", "Straight", "Three of a kind", "Two pair", "One pair", "High card" },
};

const struct game_data omaha = {
    .name = "Omaha",
    .qcards_in_deck = 52,
    .qranks = 7462,
    .qhand_categories = { 10, 156, 156, 1277, 10, 858, 858, 2860, 1277 },
    .expected_stat5 = { -1, -1, -1, -1, -1, -1, -1, -1, -1 },
    .hand_category_names = { "Straight-flush", "Four of a kind", "Full house", "Flush", "Straight", "Three of a kind", "Two pair", "One pair", "High card" },
};



/* Enumeration */

static inline int is_valid(const int qparts, const uint64_t * const masks)
{
    uint64_t total = masks[0];
    for (int i=1; i<qparts; ++i) {
        if (masks[i] & total) {
            return 0;
        }
        total |= masks[i];
    }

    return 1;
}

static inline int next_enumeration(uint64_t * restrict const data)
{
    const int qparts = data[0];

    uint64_t * restrict const masks = data + 1;
    const uint64_t * start = masks + qparts;
    const uint64_t last = start[0];

    for (;;) {
        for (int i = qparts - 1;;) {

            masks[i] = next_combination_mask(masks[i]);
            if (masks[i] < last) {
                break;
            }

            masks[i] = start[i];
            --i;

            if (i < 0) {
                return 1;
            }
        }

        if (is_valid(qparts, masks)) {
            return 0;
        }
    }
}

static inline void init_enumeration(uint64_t * restrict const data, const int * const args)
{
    const size_t data_sz = args[0];
    const int qitems = args[1];
    const int qparts = args[2];
    const int * const parts = args + 3;

    const size_t required_data_len = 2*qparts + 1;
    const size_t required_data_sz = required_data_len * sizeof(uint64_t);
    if (data_sz < required_data_sz) {
        fprintf(stderr, "Assertion failed: enumeration size %lu is not enought, %lu required\n", data_sz, required_data_sz);
        abort();
    }

    uint64_t * restrict const masks = data + 1;
    uint64_t * restrict const start = masks + qparts;
    data[0] = qparts;
    masks[0] = (1ull << parts[0]) - 1;
    start[0] = 1ull << qitems;

    for (int i=1; i<qparts; ++i) {
        masks[i] = (1ull << parts[i]) - 1;
        start[i] = masks[i];
    }

    if (!is_valid(qparts, masks)) {
        next_enumeration(data);
    }
}

static inline void test_data_init_enumeration(struct test_data * restrict const me)
{
    const int qparts = me->qcards_in_hand2 == 0 ? 1 : 2;
    const int args[] = {
        sizeof(me->enumeration),
        me->game->qcards_in_deck,
        qparts,
        me->qcards_in_hand1, me->qcards_in_hand2 };

    init_enumeration(me->enumeration, args);
}



/* Load OFSMs */

static void * load_fsm(const char * const filename, const char * const signature, const uint32_t start_from, const uint32_t qflakes, uint64_t * restrict const fsm_sz)
{
    FILE * const f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: Can not open file “%s”, error %d, %s.\n", filename, errno, strerror(errno));
        errno = 0;
        return NULL;
    }

    struct array_header header;
    const size_t header_sz = sizeof(struct array_header);
    const size_t was_read1 = fread(&header, 1, header_sz, f);
    if (was_read1 != header_sz) {
        fprintf(stderr, "Error: Can not read %lu bytes from file “%s”", header_sz, filename);
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

    const size_t sz = header.len * sizeof(uint32_t);
    void * ptr = global_malloc(sz);
    if (ptr == NULL) {
        fclose(f);
        fprintf(stderr, "Error: allocation error, malloc(%lu) failed with a NULL as a result.\n", sz);
        return NULL;
    }

    const size_t was_read2 = fread(ptr, 1, sz, f);

    if (was_read2 != sz) {
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

pack_value_t forget_suites(const unsigned int n, const input_t * const path, const unsigned int qmin)
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
            const pack_value_t tmp = nominal + 1;
            result |= tmp << shift;
            shift += 4;
        }
    }

    if (shift != 16 + 4*n) {
        fprintf(stderr, "Assertion failed: illegal shift value %d, expected %d.\n", shift, 16 + 4 * n);
    }

    return result;
}



/* Permutation generation */

static const int * get_perm_5_from_7(void)
{
    static const int * result = NULL;

    if (result != NULL) {
        return result;
    }

    const size_t sz = 5 * 22 * sizeof(int);
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

    const size_t sz = 5 * 11 * sizeof(int);
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



/* Utils */

static inline void input_to_cards(const unsigned int n, const input_t * const input, card_t * restrict const cards)
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

pack_value_t calc_six_plus_5(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 5, but %u as passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_rank5_via_robust_for_deck36(cards);
}

int create_six_plus_5(struct ofsm_builder * restrict const ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 36, 5)
        || ofsm_builder_pack(ob, calc_six_plus_5, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}



pack_value_t calc_six_plus_7(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 7, but %u was passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_six_plus_rank5_via_fsm5, cards, get_perm_5_from_7());
}

uint64_t calc_six_plus_7_flake_7_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return forget_suites(path_len, path, 4);
}

uint64_t calc_six_plus_7_flake_6_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * path)
{
    return forget_suites(path_len, path, 3);
}

int create_six_plus_7(struct ofsm_builder * restrict const ob)
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



pack_value_t calc_texas_5(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: %s requires n = 5, but %u as passed.\n", __FUNCTION__, n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_rank5_via_robust_for_deck52(cards);
}

uint64_t calc_texas_5_flake_5_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return forget_suites(path_len, path, 4);
}

int create_texas_5(struct ofsm_builder * restrict const ob)
{
    return 0
        || ofsm_builder_push_comb(ob, 52, 5)
        || ofsm_builder_pack(ob, calc_texas_5, 0)
        || ofsm_builder_optimize(ob, 5, 1, calc_texas_5_flake_5_hash)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
    ;
}



pack_value_t calc_texas_7(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: calc_six_plus_5 requires n = 7, but %u was passed.\n", n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_perm_5_from_7());
}

uint64_t calc_texas_7_flake_7_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return forget_suites(path_len, path, 4);
}

uint64_t calc_texas_7_flake_6_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return forget_suites(path_len, path, 3);
}

int create_texas_7(struct ofsm_builder * restrict const ob)
{
    load_texas_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 52, 7)
        || ofsm_builder_pack(ob, calc_texas_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 1, calc_texas_7_flake_7_hash)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



pack_value_t calc_omaha_7_flake_5_pack(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 5) {
        fprintf(stderr, "Assertion failed: %s requires n = 5, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    return forget_suites(n, input, 3);
}

pack_value_t calc_omaha_7_flake_2_pack(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 2) {
        fprintf(stderr, "Assertion failed: %s requires n = 2, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    return forget_suites(n, input, 2);
}

pack_value_t calc_omaha_7(void * const user_data, const unsigned int n, const input_t * const input)
{
    if (n != 7) {
        fprintf(stderr, "Assertion failed: %s requires n = 7, but %u was passed.\n", __FUNCTION__, n);
        abort();
    }

    card_t cards[n];
    input_to_cards(n, input, cards);
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_omaha_perm_5_from_7());
}

uint64_t calc_omaha_7_flake_7_hash(void * const user_data, const unsigned int qjumps, const state_t * const jumps, const unsigned int path_len, const input_t * const path)
{
    return forget_suites(path_len, path, 4);
}

int create_omaha_7(struct ofsm_builder * restrict const ob)
{
    load_texas_fsm5();

    return 0
        || ofsm_builder_push_comb(ob, 52, 5)
        || ofsm_builder_pack(ob, calc_omaha_7_flake_5_pack, 0)
        || ofsm_builder_optimize(ob, 5, 0, NULL)
        || ofsm_builder_push_comb(ob, 52, 2)
        || ofsm_builder_product(ob)
        || ofsm_builder_pack(ob, calc_omaha_7, PACK_FLAG__SKIP_RENUMERING)
        || ofsm_builder_optimize(ob, 7, 1, calc_omaha_7_flake_7_hash)
        || ofsm_builder_optimize(ob, 7, 0, NULL)
    ;
}



/* Library */

const char * const suite_str = "hdcs";

static inline void mask_to_cards(const int n, uint64_t mask, card_t * restrict const cards)
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

    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;
    for (int i=0; i<qcards_in_hand; ++i) {
        printf(" %s", card_str[cards[i]]);
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

static inline uint32_t test_data_eval_rank_via_fsm(const struct test_data * const me, const card_t * const cards)
{
    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;
    return eval_rank_via_fsm(qcards_in_hand, cards, me->fsm, me->game->qcards_in_deck);
}

pack_value_t eval_rank5_via_robust_for_deck36_as64(const card_t * const cards)
{
    return eval_rank5_via_robust_for_deck36(cards);
}

pack_value_t eval_rank5_via_robust_for_deck52_as64(const card_t * const cards)
{
    return eval_rank5_via_robust_for_deck52(cards);
}



/* Quick tests */

static int quick_test_for_eval_rank(struct test_data * restrict const me, const int is_robust, const card_t * const hands)
{
    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;

    user_eval_rank_f * const eval = is_robust ? me->eval_rank_robust : me->eval_rank;

    uint64_t prev_rank = 0;

    const card_t * current = hands;
    for (; *current != 0xFF; current += qcards_in_hand) {
        const uint64_t rank = eval(current);
        if (rank < prev_rank) {
            printf("[FAIL]\n");
            printf("  Wrong order!\n");

            printf("  Previous:");
            print_hand(me, current - qcards_in_hand);
            printf(" has rank %lu (0x%lX)\n", prev_rank, prev_rank);

            printf("  Current:");
            print_hand(me, current);
            printf(" has rank %lu (0x%lX)\n", rank, rank);

            return 1;
        }
        prev_rank = rank;
        ++me->counter;
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

int equivalence_check_rank(struct test_data * restrict const me, const card_t * const cards, uint64_t * restrict const saved)
{
    const uint64_t rank1 = me->eval_rank(cards);
    const uint64_t rank2 = me->eval_rank_robust(cards);

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
            return 0;
        }

        const uint64_t saved_rank = saved[rank1];
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

    return 0;
}

int test_equivalence(struct test_data * restrict const me)
{
    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;

    static uint64_t saved[9999];
    memset(saved, 0, sizeof(saved));

    test_data_init_enumeration(me);

    do {
        const uint64_t mask1 = me->enumeration[1];
        const uint64_t mask2 = me->qcards_in_hand2 > 0 ? me->enumeration[2] : 0;

        card_t cards[qcards_in_hand];
        mask_to_cards(me->qcards_in_hand1, mask1, cards);
        mask_to_cards(me->qcards_in_hand2, mask2, cards + me->qcards_in_hand1);

        const int status = equivalence_check_rank(me, cards, saved);
        if (status != 0) {
            return status;
        }

        ++me->counter;
    } while (next_enumeration(me->enumeration) == 0);

    return 0;
}



/* Permutation test */

static size_t gen_perm_table(struct test_data * restrict const me, void * ptrs[])
{
    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;

    const size_t qperm1 = factorial(me->qcards_in_hand1);
    const size_t qperm2 = factorial(me->qcards_in_hand2);
    const size_t qpermutations = qperm1 * qperm2;

    const size_t sizes[4] = {
        0, // Allign
        (qpermutations+1) * qcards_in_hand * sizeof(int8_t), // It will be used
        (qperm1+1) * me->qcards_in_hand1 * sizeof(int), // Help array 1
        (qperm2+1) * me->qcards_in_hand2 * sizeof(int), // Help array 2
    };

    multialloc(4, sizes, ptrs, 256);
    if (ptrs[0] == NULL) {
        printf("[FAIL]\n");
        printf("  multialloc(4, sizes, ptrs, 256) with sizes = { %lu, %lu, %lu, %lu } fails with NULL as ptrs[0].\n", sizes[0], sizes[1], sizes[2], sizes[3]);
        return 0;
    }

    const int q1 = gen_permutation_table(ptrs[2], me->qcards_in_hand1, sizes[2]);
    if (q1 != qperm1) {
        printf("[FAIL]\n");
        printf("  Wrong permutation count1 %d, expected value is %lu.\n", q1, qperm1);
        free(ptrs[0]);
        return 0;
    }

    if (me->qcards_in_hand2 > 0) {
        const int q2 = gen_permutation_table(ptrs[3], me->qcards_in_hand2, sizes[3]);
        if (q2 != qperm2) {
            printf("[FAIL]\n");
            printf("  Wrong permutation count2 %d, expected value is %lu.\n", q2, qperm2);
            free(ptrs[0]);
            return 0;
        }
    }

    int8_t * restrict ptr = ptrs[1];
    const int * perm1 = ptrs[2];
    for (int i1=0; i1 < qperm1; ++i1) {

        const int * perm2 = ptrs[3];
        for (int i2=0; i2 < qperm2; ++i2) {

            for (int i=0; i < me->qcards_in_hand1; ++i) {
                *ptr++ = perm1[i];
            }

            for (int i=0; i < me->qcards_in_hand2; ++i) {
                *ptr++ = perm2[i] + me->qcards_in_hand1;
            }

            perm2 += me->qcards_in_hand2;
        }

        perm1 += me->qcards_in_hand1;
    }

    for (int i=0; i<me->qcards_in_hand1; ++i) {
        *ptr++ = -1;
    }

    for (int i=0; i<me->qcards_in_hand2; ++i) {
        *ptr++ = -1;
    }

    const size_t result = ptr_diff(ptrs[1], ptr);
    if (result == 0) {
        printf("[FAIL]\n");
        printf("  Wrong result = 0.\n");
        free(ptrs[0]);
        return 0;
    }

    return result;
}

static inline void apply_perm(const int n, card_t * restrict const dest, const card_t * const src, const int8_t * const perm)
{
    for (int i=0; i<n; ++i) {
        dest[i] = src[perm[i]];
    }
}

int test_permutations(struct test_data * restrict const me)
{
    void * perm_ptrs[4];
    const size_t perm_table_sz = gen_perm_table(me, perm_ptrs);
    if (perm_table_sz == 0) {
        return 1;
    }
    const int8_t * const perm_table = perm_ptrs[1];

    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;

    test_data_init_enumeration(me);

    if (opt_opencl) {
        me->is_opencl = 1;

        const unsigned int qparts = me->enumeration[0];
        const size_t datum_sz = sizeof(uint64_t) * qparts;
        const size_t data_chunk_sz = 1024 * 1024 * 32; // 32 Mb
        const size_t chunk_len = data_chunk_sz / datum_sz;
        const size_t data_sz = chunk_len * datum_sz;
        const size_t report_sz = chunk_len * sizeof(uint16_t);
        const size_t sizes[3] = { 0, data_sz, report_sz };
        void * ptrs[3];
        multialloc(3, sizes, ptrs, 256);
        if (ptrs[0] == NULL) {
            printf("[FAIL] (OpenCL)\n");
            printf("  multialloc(3, sizes, ptrs, 256) where sizes = { 0, %lu, %lu } failed with NULL as ptrs[0].\n", data_sz, report_sz);
            free(perm_ptrs[0]);
            return 1;
        }
        uint16_t * restrict const report = ptrs[2];

        struct opencl_permunation_args args = {
            .qdata = 0,
            .n = qcards_in_hand,
            .start_state = me->game->qcards_in_deck,
            .qparts = qparts,
            .perm_table = perm_table,
            .perm_table_sz = perm_table_sz,
            .fsm = me->fsm,
            .fsm_sz = me->fsm_sz,
        };

        void * const opencl_test = create_opencl_permutations(&args);
        if (opencl_test == NULL) {
            free(ptrs[0]);
            free(perm_ptrs[0]);
            return 1;
        }

        int status = 0;
        int64_t qdata = 0;
        uint64_t * restrict data = ptrs[1];
        for (;;) {
            memcpy(data, me->enumeration + 1, datum_sz);
            data += qparts;
            ++qdata;

            const int enumeration_status = next_enumeration(me->enumeration);

            if (qdata == chunk_len || enumeration_status != 0) {

                data = ptrs[1];

                uint64_t qerrors;
                status = run_opencl_permutations(opencl_test, qdata, data, report, &qerrors);
                if (status != 0) {
                    break;
                }

                me->counter += qdata;

                int64_t idx_report = -1;
                for (uint32_t i=0; i<qdata; ++i) {
                    if (report[i] != 0) {
                        idx_report = i;
                        break;
                    }
                }

                if (idx_report >= 0 || qerrors > 0) {
                    printf("[FAIL] (OpenCL)\n");
                    printf("  qerrors = %lu\n", qerrors);

                    if (idx_report >= 0) {
                        const int64_t idx_data = idx_report * qparts;
                        printf("  report[%ld] = %u is nonzero.\n", idx_report, report[idx_report]);
                        printf("  data[%ld] = 0x%016lx.\n", idx_data, data[idx_data]);
                        if (qparts > 1) {
                            printf("  data[%ld] = 0x%016lx.\n", idx_data+1, data[idx_data+1]);
                        }

                        card_t cards[qcards_in_hand];
                        mask_to_cards(me->qcards_in_hand1, data[idx_data], cards);
                        if (qparts > 1) {
                            mask_to_cards(me->qcards_in_hand2, data[idx_data+1], cards + me->qcards_in_hand1);
                        }

                        const uint32_t r = test_data_eval_rank_via_fsm(me, cards);
                        printf("  Base Hand:");
                        print_hand(me, cards);
                        printf(" has rank %u\n", r);
                    }

                    status = 1;
                    break;
                }

                qdata = 0;
            }

            if (enumeration_status != 0) {
                break;
            }

        }

        free_opencl_permutations(opencl_test);
        free(ptrs[0]);
        free(perm_ptrs[0]);
        return status;
    }

    do {
        const uint64_t mask1 = me->enumeration[1];
        const uint64_t mask2 = me->qcards_in_hand2 > 0 ? me->enumeration[2] : 0;

        card_t cards[qcards_in_hand];
        mask_to_cards(me->qcards_in_hand1, mask1, cards);
        mask_to_cards(me->qcards_in_hand2, mask2, cards + me->qcards_in_hand1);

        const uint32_t rank = test_data_eval_rank_via_fsm(me, cards);

        for (const int8_t * perm = perm_table; *perm >= 0; perm += qcards_in_hand) {
            card_t c[qcards_in_hand];
            apply_perm(qcards_in_hand, c, cards, perm);
            const uint32_t r = test_data_eval_rank_via_fsm(me, cards);

            if (r != rank) {
                printf("[FAIL]\n");

                printf("  Base Hand:");
                print_hand(me, cards);
                printf(" has rank %u\n", rank);

                printf("  Current Hand: ");
                print_hand(me, c);
                printf(" has rank %u\n", r);
                free(perm_ptrs[0]);
                return 1;
            }
        }

        ++me->counter;

    } while (next_enumeration(me->enumeration) == 0);

    free(perm_ptrs[0]);
    return 0;
}



/* Stat test */

int test_stats(struct test_data * restrict const me)
{
    const int qcards_in_hand = me->qcards_in_hand1 + me->qcards_in_hand2;

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

    test_data_init_enumeration(me);

    do {
        const uint64_t mask1 = me->enumeration[1];
        const uint64_t mask2 = me->qcards_in_hand2 > 0 ? me->enumeration[2] : 0;

        card_t cards[qcards_in_hand];
        mask_to_cards(me->qcards_in_hand1, mask1, cards);
        mask_to_cards(me->qcards_in_hand2, mask2, cards + me->qcards_in_hand1);

        const uint32_t rank = me->eval_rank(cards);
        if (rank < 0 || rank > qranks) {
            printf("[FAIL]\n");
            printf("Invalid rank = %u for hand:", rank);
            print_hand(me, cards);
            printf("\n");
            return 1;
        }

        ++stats[rank];
        ++me->counter;

    } while (next_enumeration(me->enumeration) == 0);

    if (qcards_in_hand == 5) {
        for (int i=1; i<=qranks; ++i) {
            if (stats[i] == 0) {
                printf("[FAIL]\n");
                printf("Unexpected stats[%d] = 0.\n", i);
                return 1;
            }
        }
    }

    int * restrict const hand_type_stats = me->hand_type_stats;

    for (int i=0; i<9; ++i) {
        hand_type_stats[i] = 0;
        const int lo = hand_category_low_ranges[i];
        const int hi = lo + qhand_categories[i];
        for (int j=lo; j<hi; ++j) {
            hand_type_stats[i] += stats[j];
        }
    }

    if (qcards_in_hand == 5) {
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
            printf("    %*s %10d  %4.1f%%\n",
                -16, me->game->hand_category_names[i],
                me->hand_type_stats[i], 100.0 * me->hand_type_stats[i] / total
            );
        }
        printf("   ------------------------------------\n");
        printf("    Total            %10lu 100.0%%\n", total);
    }
}



/* Test environment */

typedef int test_function(struct test_data * restrict const);

static inline int run_test(struct test_data * restrict const me, const char * const name, test_function test)
{
    const int w = -66;

    const double start = get_app_age();
    printf("  Run %*s ", w, name);
    fflush(stdout);

    me->is_opencl = 0;
    me->counter = 0;
    const int status = test(me);
    if (status != 0) {
        return 1;
    }
    const double delta = get_app_age() - start;
    printf("[ OK ] %lu in %.2f s.%s\n", me->counter, delta, me->is_opencl ? " (OpenCL)" : "");
    return 0;
}

#define RUN_TEST(desc, f)                          \
    do {                                           \
        int err = run_test(desc, #f "() ...", f);  \
        if (err) return 1;                         \
    } while (0)                                    \



/* Six Plus fsm5 tests */

static int quick_test_six_plus_eval_rank5_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck36);
}

static int quick_test_six_plus_eval_rank5(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck36);
}

static uint64_t eval_six_plus_rank5_via_fsm5_as64(const card_t * const cards)
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
        .qcards_in_hand1 = 5,
        .qcards_in_hand2 = 0,
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

static int quick_test_six_plus_eval_rank7_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand7_for_deck36);
}

static int quick_test_six_plus_eval_rank7(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand7_for_deck36);
}

static uint64_t eval_six_plus_rank7_via_fsm7_as64(const card_t * const cards)
{
    return eval_six_plus_rank7_via_fsm7(cards);
}

static uint64_t eval_six_plus_rank7_via_fsm5_brutte_as64(const card_t * const cards)
{
    return eval_via_perm(eval_six_plus_rank5_via_fsm5, cards, get_perm_5_from_7());
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
        .qcards_in_hand1 = 7,
        .qcards_in_hand2 = 0,
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

static int quick_test_texas_eval_rank5_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand5_for_deck52);
}

static int quick_test_texas_eval_rank5(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand5_for_deck52);
}

static uint64_t eval_texas_rank5_via_fsm5_as64(const card_t * const cards)
{
    return eval_texas_rank5_via_fsm5(cards);
}

int check_texas_5(void)
{
    printf("Texas 5 tests:\n");

    load_texas_fsm5();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &texas_holdem,
        .qcards_in_hand1 = 5,
        .qcards_in_hand2 = 0,
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

static uint64_t eval_texas_rank7_via_fsm7_as64(const card_t * const cards)
{
    return eval_texas_rank7_via_fsm7(cards);
}

static uint64_t eval_texas_rank7_via_fsm5_brutte_as64(const card_t * const cards)
{
    return eval_via_perm(eval_texas_rank5_via_fsm5, cards, get_perm_5_from_7());
}

int check_texas_7(void)
{
    printf("Texas 7 tests:\n");

    load_texas_fsm5();
    load_texas_fsm7();

    int hand_type_stats[9];
    memset(hand_type_stats, 0, sizeof(hand_type_stats));

    struct test_data suite = {
        .game = &texas_holdem,
        .qcards_in_hand1 = 7,
        .qcards_in_hand2 = 0,
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

static int quick_test_omaha_eval_robust(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 1, quick_ordered_hand7_for_omaha);
}

static int quick_test_omaha_eval(struct test_data * restrict const me)
{
    return quick_test_for_eval_rank(me, 0, quick_ordered_hand7_for_omaha);
}

static uint64_t eval_omaha_rank7_via_fsm7_as64(const card_t * cards)
{
    return eval_omaha_rank7_via_fsm7(cards);
}

static uint64_t eval_omaha_rank7_via_fsm5_brutte_as64(const card_t * cards)
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
        .qcards_in_hand1 = 5,
        .qcards_in_hand2 = 2,
        .strict_equivalence = 1,
        .eval_rank = eval_omaha_rank7_via_fsm7_as64,
        .eval_rank_robust = eval_omaha_rank7_via_fsm5_brutte_as64,
        .fsm = omaha_fsm7,
        .fsm_sz = omaha_fsm7_sz,
        .hand_type_stats = hand_type_stats
    };

    RUN_TEST(&suite, quick_test_omaha_eval_robust);
    RUN_TEST(&suite, quick_test_omaha_eval);
    RUN_TEST(&suite, test_equivalence);
    RUN_TEST(&suite, test_permutations);
    RUN_TEST(&suite, test_stats);

    print_stats(&suite);
    printf("All omaha 7 tests are successfully passed.\n");
    return 0;
}



/* Debug code */

int debug_something(void)
{
    return 0;
}
