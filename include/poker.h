#ifndef YOO__POKER__H__
#define YOO__POKER__H__

#include <stdio.h>



#define CARD(nominal, suite)  (nominal*4 + suite)
#define SUITE(card)           (card & 3)
#define NOMINAL(card)         (card >> 2)



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



int run_create_six_plus_5(struct ofsm_builder * restrict ob);
int run_check_six_plus_5(void);

int run_create_six_plus_7(struct ofsm_builder * restrict ob);
int run_check_six_plus_7(void);

#endif
