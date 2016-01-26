#include "fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int empty_test();

#define TEST_ITEM(name) { #name, &name##_test, 0 }

struct test_item tests[] = {
    TEST_ITEM(empty),
    { NULL, NULL }
};

struct test_item * current_test;

void print_tests()
{
    struct test_item * current = tests;
    for (; current->name != NULL; ++current) {
        int might_be_printed = 0
           || (!current->is_slow && opt_print_common)
           || (current->is_slow && opt_print_slow)
        ;
        if (might_be_printed) {
            printf("%s\n", current->name);
        }
    }
}

void run_test_item(const struct test_item * item)
{
    printf("Run test “%s”.\n", item->name);
    int code = (*item->func)();
    if (code) {
        exit(code);
    }
}

void run_test(const char * name)
{
    for (current_test = tests; current_test->name != NULL; ++current_test) {
        if (strcmp(name, current_test->name) == 0) {
            return run_test_item(current_test);
        }
    }

    fprintf(stderr, "Test “%s” is not found.\n", name);
    exit(1);
}

int empty_test()
{
    return 0;
}

int main(int argc, char * argv[])
{
    if (argc <= 1) {
        print_tests();
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        run_test(argv[i]);
    }

    return 0;
}
