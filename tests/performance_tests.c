#include <time.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <inttypes.h>
 
#include "ast.h" 
#include "parser.h" 
#include "minunit.h" 
#include "utils.h"
 
int parse(const char *text, struct ast_node **node); 
 
#define COUNT 100
 
int test_cdir_split() 
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, COUNT, false);

    const char* data[COUNT]; 

    for(size_t i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a = %zu", i); 
        data[i] = expr; 
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(size_t i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(config, i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(size_t i = 0; i < COUNT; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event_i(config, "a", 0); 
    struct matched_subs* matched_subs = make_matched_subs(); 
    match_be_tree(config, event, cnode, matched_subs); 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(matched_subs->sub_count == 1, "Found our sub");

    uint64_t init_us = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t parse_us = (parse_done.tv_sec - init_done.tv_sec) * 1000000 + (parse_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - parse_done.tv_sec) * 1000000 + (insert_done.tv_nsec - parse_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000 + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Parse took %" PRIu64 "\n", parse_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    for(size_t i = 0; i < COUNT; i++) { 
        free((char*)data[i]); 
    } 
    free_config(config); 
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
} 

int test_pdir_split()
{
    struct config* config = make_default_config();

    const char* data[COUNT]; 

    for(size_t i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a%zu = 0", i); 
        data[i] = expr; 

        char* name;
        asprintf(&name, "a%zu", i);
        add_attr_domain_i(config, name, 0, 10, false);
        free(name);
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(size_t i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(config, i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(size_t i = 0; i < COUNT; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event_i(config, "a0", 0); 
    struct matched_subs* matched_subs = make_matched_subs(); 
    match_be_tree(config, event, cnode, matched_subs); 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    mu_assert(matched_subs->sub_count == 1, "Found our sub");

    uint64_t init_us = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t parse_us = (parse_done.tv_sec - init_done.tv_sec) * 1000000 + (parse_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - parse_done.tv_sec) * 1000000 + (insert_done.tv_nsec - parse_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000 + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Parse took %" PRIu64 "\n", parse_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    for(size_t i = 0; i < COUNT; i++) { 
        free((char*)data[i]); 
    } 
    free_config(config);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
}

#include <limits.h>

const struct sub* get_sub(const struct sub** subs, size_t sub_count, betree_sub_t sub_id)
{
    for(size_t i = 0; i < sub_count; i++) {
        const struct sub* sub = subs[i];
        if(sub->id == sub_id) {
            return sub;
        }
    }
    return NULL;
}

void fill_event_random(const struct sub** subs, size_t sub_count, struct event* event, size_t count) 
{
    event->pred_count = count;
    event->preds = calloc(count, sizeof(*event->preds));
    if(event->preds == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    for(size_t i = 0; i < count; i++) {
        size_t sub_index = random_in_range(0, sub_count - 1);
        const struct sub* sub = subs[sub_index];
        size_t variable_id_index = random_in_range(0, sub->variable_id_count - 1);
        betree_var_t variable_id = sub->variable_ids[variable_id_index];
        int64_t value = random_in_range(0, 100);
        struct pred* pred = (struct pred*)make_simple_pred_i(variable_id, value);
        event->preds[i] = pred;
    }
}

int test_complex()
{
    struct config* config = make_default_config();

    struct timespec start, init_done, parse_done, insert_done, gen_event_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    FILE* f = fopen("output.be", "r");
    mu_assert(f != NULL, "can open output.be");

    char line[LINE_MAX];
    struct ast_node* node; 
    size_t sub_count = 0;
    struct sub** subs;
    while(fgets(line, sizeof(line), f)) {
        if(parse(line, &node) != 0) {
            printf("Failed to parse: %s\n", line);
            return 1;
        }
        adjust_attr_domains_i(config, node, 0, 100, false);
        const struct sub* sub = make_sub(config, sub_count + 1, node); 
        if(sub_count == 0) {
            subs = calloc(1, sizeof(*subs));
            if(subs == NULL) {
                fprintf(stderr, "%s calloc failed", __func__);
                abort();
            }
        }
        else {
            struct sub** next_subs = realloc(subs, sizeof(*next_subs) * (sub_count + 1));
            if(next_subs == NULL) {
                fprintf(stderr, "%s realloc failed", __func__);
                abort();
            }
            subs = next_subs;
        }
        subs[sub_count] = (struct sub*)sub;
        sub_count++;
    }

    fclose(f);

    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(size_t i = 0; i < sub_count; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    struct event* event = (struct event*)make_event();
    srand((unsigned int)time(NULL));
    fill_event_random(subs, sub_count, event, 5);
    
    clock_gettime(CLOCK_MONOTONIC_RAW, &gen_event_done);

    struct matched_subs* matched_subs = make_matched_subs(); 
    match_be_tree(config, event, cnode, matched_subs); 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    uint64_t init_us = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t parse_us = (parse_done.tv_sec - init_done.tv_sec) * 1000000 + (parse_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - parse_done.tv_sec) * 1000000 + (insert_done.tv_nsec - parse_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - gen_event_done.tv_sec) * 1000000 + (search_done.tv_nsec - gen_event_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Parse took %" PRIu64 "\n", parse_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

    char event_str[LINE_MAX];
    event_to_string(config, event, event_str);
    printf("    Event: %s\n", event_str);
    if(matched_subs->sub_count > 0) {
        printf("    Matched subs:\n");
        for(size_t i = 0; i < matched_subs->sub_count; i++) {
            betree_sub_t sub_id = matched_subs->subs[i];
            const struct sub* sub = get_sub(subs, sub_count, sub_id);
            const char* expr = ast_to_string(sub->expr);
            printf("    %llu: %s", sub->id, expr);
            free((char*)expr);
        }
    }
    else {
        printf("    No matched subs\n");
    }

    free(subs);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    free_config(config);
    return 0; 
}

int all_tests()  
{ 
    mu_run_test(test_cdir_split); 
    printf("\n");
    mu_run_test(test_pdir_split);
    printf("\n");
    mu_run_test(test_complex);
 
    return 0; 
} 
 
RUN_TESTS() 