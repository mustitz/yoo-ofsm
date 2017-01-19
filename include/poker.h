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
extern const card_t quick_ordered_hand7_for_omaha[];

uint64_t eval_rank5_via_robust_for_deck36(const card_t * cards);
uint64_t eval_rank5_via_robust_for_deck52(const card_t * cards);


extern int opt_opencl;

int init_opencl(FILE * err);
void free_opencl(void);

struct opencl_permunation_args {
    uint64_t qdata;
    uint64_t n;
    uint64_t start_state;
    uint64_t qparts;

    const int8_t * const perm_table;
    const uint64_t perm_table_sz;

    const uint32_t * const fsm;
    const uint64_t fsm_sz;
};

void * create_opencl_permutations(const struct opencl_permunation_args * const args);
void free_opencl_permutations(void * handle);
int run_opencl_permutations(
    void * handle,
    uint64_t qdata,
    const uint64_t * const data,
    uint16_t * restrict const report,
    uint64_t * restrict const qerrors
);

int opencl__test_permutations(
    uint64_t * restrict args,
    const int8_t * const perm_table, const int64_t perm_table_sz,
    const uint32_t * const fsm, const int64_t fsm_sz,
    const uint64_t * const data, const int64_t data_sz,
    uint16_t * restrict const report, const int64_t report_sz
);



void * global_malloc(size_t sz);
void global_free(void);



typedef int (*create_func)(struct ofsm_builder * restrict const);
typedef int (*check_func)();

struct poker_ofsm
{
    const char * name;
    const char * file_name;
    const char * signature;
    int delta;
    create_func create;
    check_func check;
};



int create_six_plus_5(struct ofsm_builder * restrict ob);
int check_six_plus_5(void);

int create_six_plus_7(struct ofsm_builder * restrict ob);
int check_six_plus_7(void);

int create_texas_5(struct ofsm_builder * restrict ob);
int check_texas_5(void);

int create_texas_7(struct ofsm_builder * restrict ob);
int check_texas_7(void);

int create_omaha_7(struct ofsm_builder * restrict ob);
int check_omaha_7(void);

void save_binary(const char * file_name, const char * name, const struct ofsm_array * array);
int debug_something(void);

#endif
