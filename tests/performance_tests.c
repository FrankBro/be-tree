#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alloc.h"
#include "betree.h"
#include "debug.h"
#include "minunit.h"
#include "utils.h"

#define COUNT 1000

int test_cdir_split()
{
    struct betree* tree = betree_make();
    betree_add_integer_variable(tree, "a", false, 0, COUNT);

    struct timespec start, init_done, insert_done, search_done;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    for(size_t i = 0; i < COUNT; i++) {
        char* expr;
        if(basprintf(&expr, "a = %zu", i) < 0) {
            abort();
        }
        betree_insert(tree, i + 1, expr);
        free(expr);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    struct report* report = make_report();
    const char* event = "{\"a\": 0}";
    mu_assert(betree_search(tree, event, report), "");

    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(report->matched == 1, "Found our sub");

    uint64_t init_us
        = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - init_done.tv_sec) * 1000000
        + (insert_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000
        + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_pdir_split()
{
    struct betree* tree = betree_make();

    for(size_t i = 0; i < COUNT; i++) {
        char* name;
        if(basprintf(&name, "a%zu", i) < 0) {
            abort();
        }
        betree_add_integer_variable(tree, name, true, 0, 10);
        free(name);
    }

    struct timespec start, init_done, insert_done, search_done;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    for(size_t i = 0; i < COUNT; i++) {
        char* expr;
        if(basprintf(&expr, "a%zu = 0", i) < 0) {
            abort();
        }
        betree_insert(tree, i + 1, expr);
        free(expr);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const char* event = "{\"a0\": 0}";
    struct report* report = make_report();
    mu_assert(betree_search(tree, event, report), "");

    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(report->matched == 1, "Found our sub");

    uint64_t init_us
        = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - init_done.tv_sec) * 1000000
        + (insert_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000
        + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    free_report(report);
    betree_free(tree);
    return 0;
}

int all_tests()
{
    mu_run_test(test_cdir_split);
    printf("\n");
    mu_run_test(test_pdir_split);
    printf("\n");

    return 0;
}

RUN_TESTS()
