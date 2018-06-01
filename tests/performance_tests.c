#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ast.h"
#include "debug.h"
#include "minunit.h"
#include "utils.h"

int parse(const char* text, struct ast_node** node);

#define COUNT 1000

int test_cdir_split()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, COUNT, false);

    struct timespec start, init_done, insert_done, search_done;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    struct cnode* cnode = make_cnode(config, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    for(size_t i = 0; i < COUNT; i++) {
        char* expr;
        asprintf(&expr, "a = %zu", i);
        betree_insert(config, i + 1, expr, cnode);
        free(expr);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const char* event = "{\"a\": 0}";
    struct matched_subs* matched_subs = make_matched_subs();
    betree_search(config, event, cnode, matched_subs, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(matched_subs->sub_count == 1, "Found our sub");

    uint64_t init_us
        = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - init_done.tv_sec) * 1000000
        + (insert_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000
        + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    free_config(config);
    free_matched_subs(matched_subs);
    free_cnode(cnode);
    return 0;
}

extern bool MATCH_NODE_DEBUG;

int test_pdir_split()
{
    struct config* config = make_default_config();

    for(size_t i = 0; i < COUNT; i++) {
        char* name;
        asprintf(&name, "a%zu", i);
        add_attr_domain_i(config, name, 0, 10, true);
        free(name);
    }

    struct timespec start, init_done, insert_done, search_done;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    struct cnode* cnode = make_cnode(config, NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    for(size_t i = 0; i < COUNT; i++) {
        char* expr;
        asprintf(&expr, "a%zu = 0", i);
        betree_insert(config, i + 1, expr, cnode);
        free(expr);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const char* event = "{\"a0\": 0}";
    struct matched_subs* matched_subs = make_matched_subs();
    betree_search(config, event, cnode, matched_subs, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(matched_subs->sub_count == 1, "Found our sub");

    uint64_t init_us
        = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - init_done.tv_sec) * 1000000
        + (insert_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000
        + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    free_config(config);
    free_matched_subs(matched_subs);
    free_cnode(cnode);
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
