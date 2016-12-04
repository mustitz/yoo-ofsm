#include "yoo-ofsm.h"

#include "poker.h"

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "../arrays/fsm5.c"
const unsigned int * const fsm5 = fsm5_data;


void build_holdem_5(void * script);
int check_holdem_5(const void * ofsm);

void build_six_plus_7(void * script);
int check_six_plus_7(const void * ofsm);

const char * card_str[] = {
    "2s", "2c", "2d", "2h",
    "3s", "3c", "3d", "3h",
    "4s", "4c", "4d", "4h",
    "5s", "5c", "5d", "5h",
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


uint64_t minmax_hash(unsigned int n, const state_t * jumps)
{
    uint64_t max = 0;
    uint64_t min = ~max;
    for (unsigned int i = 0; i < n; ++i) {
        if (jumps[i] == INVALID_STATE) continue;
        if (jumps[i] < min) min = jumps[i];
        if (jumps[i] > max) max = jumps[i];
    }
    return min + 100000 * max;
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

static inline unsigned int eval_enum_omaha_nrank7(const card_t * cards)
{
    unsigned int result = 0;
    unsigned int start = 52;

    unsigned int s0 = fsm5[start + cards[0]];
    unsigned int s1 = fsm5[start + cards[1]];
    unsigned int s2 = fsm5[start + cards[2]];

    #define NEW_STATE(x, y) unsigned int x##y = fsm5[x + cards[y]];
    #define LAST_STATE(x, y) { unsigned int tmp = fsm5[x + cards[y]]; if (tmp > result) result = tmp; }

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

    NEW_STATE(s012, 5);
    NEW_STATE(s013, 5);
    NEW_STATE(s014, 5);
    NEW_STATE(s023, 5);
    NEW_STATE(s024, 5);
    NEW_STATE(s034, 5);
    NEW_STATE(s123, 5);
    NEW_STATE(s124, 5);
    NEW_STATE(s134, 5);
    NEW_STATE(s234, 5);

    LAST_STATE(s0125, 6);
    LAST_STATE(s0135, 6);
    LAST_STATE(s0145, 6);
    LAST_STATE(s0235, 6);
    LAST_STATE(s0245, 6);
    LAST_STATE(s0345, 6);
    LAST_STATE(s1235, 6);
    LAST_STATE(s1245, 6);
    LAST_STATE(s1345, 6);
    LAST_STATE(s2345, 6);

    #undef NEW_STATE
    #undef LAST_STATE

    return result;
}

pack_value_t calc_omaha7(unsigned int n, const input_t * path)
{
    card_t cards[7] = { path[0], path[1], path[2], path[3], path[4], path[5], path[6] };
    pack_value_t result = eval_enum_omaha_nrank7(cards);
    if (result <= 0 || result > 7462) {
        fprintf(stderr, "Assertion failed: invalid omaha7 nrank: %s %s %s %s %s %s %s -> %lu.\n",
            card_str[cards[0]],
            card_str[cards[1]],
            card_str[cards[2]],
            card_str[cards[3]],
            card_str[cards[4]],
            card_str[cards[5]],
            card_str[cards[6]],
            result
            );
        abort();
    }

    return result;
}

void build_omaha7_script(void * script)
{
    script_step_comb(script, 52, 5);
    script_step_pack(script, omaha_simplify_5, 0);
    script_step_comb(script, 52, 2);
    script_step_pack(script, calc_omaha7, PACK_FLAG__SKIP_RENUMERING);
    script_step_optimize(script, 7, minmax_hash);
    script_step_optimize(script, 7, NULL);
    script_step_optimize(script, 6, NULL);
    script_step_optimize(script, 5, NULL);
    script_step_optimize(script, 4, NULL);
    script_step_optimize(script, 3, NULL);
    script_step_optimize(script, 2, NULL);
    script_step_optimize(script, 1, NULL);
}

int check_omaha7(const void * ofsm)
{
    struct ofsm_array array;
    int errcode = ofsm_get_array(ofsm, 0, &array);
    if (errcode != 0) {
        fprintf(stderr, "ofsm_get_array(ofsm, 0, &array) failed with %d as error code.\n", errcode);
        return 1;
    }

    ofsm_print_array(stdout, "omaha7_data", &array, 52);
    save_binary("omaha7.bin", "OFSM Omaha 7", &array);

    free(array.array);
    return 0;
}



struct selector
{
    const char * name;
    build_script_func * build;
    check_ofsm_func * check;
};

struct selector selectors[] = {
    { "six-plus-7", build_six_plus_7, check_six_plus_7 },
    { "holdem-5", build_holdem_5, check_holdem_5 },
    { "omaha7", build_omaha7_script, check_omaha7 },
    { NULL, NULL }
};

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



int opt_check = 0;
int opt_verbose = 0;
int opt_help = 0;



struct ofsm_builder * create_ob(void)
{
    struct ofsm_builder * restrict ob = create_ofsm_builder(NULL, stderr);
    if (ob == NULL) return NULL;

    if (opt_verbose) {
        ob->logstream = stdout;
    }

    return ob;
}



int run_create_six_plus_5(struct ofsm_builder * restrict ob);
static int create_six_plus_5(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating six-plus-5...\n");
    int err = run_create_six_plus_5(ob);

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 1, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.", err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("six-plus-5.bin", "OFSM Six Plus 5", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}

int run_check_six_plus_5(void);
static int check_six_plus_5(void)
{
    return run_check_six_plus_5();
}

int run_create_six_plus_7(struct ofsm_builder * restrict ob);
static int create_six_plus_7(void)
{
    struct ofsm_builder * restrict const ob = create_ob();
    if (ob == NULL) return 1;

    printf("%s", "Creating six-plus-7...\n");
    int err = run_create_six_plus_7(ob);

    struct ofsm_array array;
    err = ofsm_builder_make_array(ob, 1, &array);
    if (err != 0) {
        fprintf(stderr, "ofsm_builder_get_array(me) failed with %d as error code.", err);
        free_ofsm_builder(ob);
        return 1;
    }

    save_binary("six-plus-7.bin", "OFSM Six Plus 7", &array);

    free(array.array);
    free_ofsm_builder(ob);
    return err;
}



static void usage(void)
{
    printf("%s",
        "USAGE: yoo-build-poker-ofsm [OPTION] table1 [table2 ... tableN]\n"
        "  --help, -h      Print usage and terminate.\n"
        "  --verbose, -v   Output an extended logging information to stderr.\n"
    );
}

static int parse_command_line(int argc, char * argv[])
{
    static struct option long_options[] = {
        { "check", no_argument, &opt_check, 1 },
        { "help",  no_argument, &opt_help, 1},
        { "verbose", no_argument, &opt_verbose, 1 },
        { NULL, 0, NULL, 0 }
    };

    for (;;) {
        int index = 0;
        const int c = getopt_long(argc, argv, "hcv", long_options, &index);
        if (c == -1) break;

        if (c != 0) {
            switch (c) {
                case 'c':
                    opt_check = 1;
                    break;
                case 'h':
                    opt_help = 1;
                    break;
                case 'v':
                    opt_verbose = 1;
                    break;
                 case '?':
                    fprintf(stderr, "Invalid option.\n");
                    return -1;
                default:
                    fprintf(stderr, "getopt_long returns unexpected char \\x%02X.\n", c);
                    return -1;
            }
        }
    }

    return optind;
}



static int create_holdem_5(void)
{
    fprintf(stderr, "Not implemented create_holdem_5();\n");
    return 1;
}

static int create_omaha_7(void)
{
    fprintf(stderr, "Not implemented create_omaha_7();\n");
    return 1;
}



typedef int (*create_func)(void);

struct poker_table
{
    const char * name;
    create_func create;
    create_func check;
};

static int stub(void)
{
    return 0;
}

struct poker_table poker_tables[] = {
    { "six-plus-5", create_six_plus_5, check_six_plus_5 },
    { "six-plus-7", create_six_plus_7, stub },
    { "holdem-5", create_holdem_5, stub },
    { "omaha-7", create_omaha_7, stub },
    { NULL, NULL, NULL }
};

static void print_table_names(void)
{
    const struct poker_table * entry = poker_tables;
    for (;; ++entry) {
        if (entry->name == NULL) {
            break;
        }
        printf("%s\n", entry->name);
    }
}

int main(int argc, char * argv[])
{
    const int first_arg = parse_command_line(argc, argv);

    if (first_arg < 0) {
        usage();
        return 1;
    }

    if (opt_help) {
        usage();
        return 0;
    }

    if (first_arg >= argc) {
        print_table_names();
        return 0;
    }

    int qerrors = 0;
    const int qcalls = argc - first_arg;
    create_func calls[qcalls];
    for (int i=0; i<qcalls; ++i) {
        const char * const table_name = argv[first_arg + i];
        const struct poker_table * entry = poker_tables;
        for (;; ++entry) {
            if (entry->name == NULL) {
                ++qerrors;
                fprintf(stderr, "Table name “%s” is not found.\n", table_name);
                break;
            }
            if (strcmp(table_name, entry->name) == 0) {
                calls[i] = opt_check ? entry->check : entry->create;
                break;
            }
        }
    }

    if (qerrors > 0) {
        return 1;
    }

    for (int j=0; j<qcalls; ++j) {
        int err = calls[j]();
        if (err != 0) {
            return 1;
        }
    }

    return 0;
}
