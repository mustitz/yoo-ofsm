#ifndef YOO__POKER__H__
#define YOO__POKER__H__

#include <stdint.h>
#include <stdio.h>

#include <yoo-ofsm.h>



typedef uint32_t card_t;

#define CARD(nominal, suite)  (nominal*4 + suite)
#define SUITE(card)           (card & 3)
#define NOMINAL(card)         (card >> 2)

#define NOMINAL_MASK(n)  (1 << NOMINAL_##n)
#define NOMINAL_MASK_2(n1, n2)  (NOMINAL_MASK(n1) | NOMINAL_MASK(n2))
#define NOMINAL_MASK_3(n1, n2, n3)  (NOMINAL_MASK_2(n1, n2) | NOMINAL_MASK(n3))
#define NOMINAL_MASK_4(n1, n2, n3, n4)  (NOMINAL_MASK_3(n1, n2, n3) | NOMINAL_MASK(n4))
#define NOMINAL_MASK_5(n1, n2, n3, n4, n5)  (NOMINAL_MASK_4(n1, n2, n3, n4) | NOMINAL_MASK(n5))

#define SUITE_S    3
#define SUITE_C    2
#define SUITE_D    1
#define SUITE_H    0



extern const char * suite_str;
extern const char * card36_str[];
extern const char * nominal36_str;
extern const char * card52_str[];
extern const char * nominal52_str;

extern const card_t quick_ordered_hand5_for_deck36[];
extern const card_t quick_ordered_hand7_for_deck36[];
extern const card_t quick_ordered_hand5_for_deck52[];
extern const card_t quick_ordered_hand7_for_deck52[];

uint64_t eval_rank5_via_robust_for_deck36(const card_t * cards);
uint64_t eval_rank5_via_robust_for_deck52(const card_t * cards);


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



void * global_malloc(size_t sz);
void global_free(void);



struct poker_table;

typedef int (*create_func)(const struct poker_table * const);
typedef int (*check_func)(void);

struct poker_table
{
    const char * name;
    create_func create;
    check_func check;
    int delta;
};



int run_create_six_plus_5(struct ofsm_builder * restrict ob);
int run_check_six_plus_5(void);

int run_create_six_plus_7(struct ofsm_builder * restrict ob);
int run_check_six_plus_7(void);

int run_create_texas_5(struct ofsm_builder * restrict ob);
int run_check_texas_5(void);

int run_create_texas_7(struct ofsm_builder * restrict ob);
int run_check_texas_7(void);

int run_create_test(struct ofsm_builder * restrict ob);
int run_check_test(void);

#endif
