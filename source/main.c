#include <ofsm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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



pack_value_t calc_rank(unsigned int n, const input_t * path)
{
    return 0;
}

void build_default_script(void * script)
{
    script_step_comb(script, 52, 5);
    script_step_pack(script, calc_rank);
    script_step_optimize(script, 5, NULL);
    script_step_optimize(script, 4, NULL);
    script_step_optimize(script, 3, NULL);
    script_step_optimize(script, 2, NULL);
    script_step_optimize(script, 1, NULL);
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

void build_omaha_script(void * script)
{
    script_step_comb(script, 52, 5);
    script_step_pack(script, omaha_simplify_5);
    script_step_comb(script, 52, 2);
}

int main(int argc, char * argv[])
{
    if (is_prefix("omaha", &argc, argv)) {
        return execute(argc, argv, build_omaha_script, NULL);
    }

    return execute(argc, argv, build_default_script, NULL);
}
