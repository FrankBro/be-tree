#include <time.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <inttypes.h>
 
#include "ast_parse.h" 
#include "parser.h" 
#include "minunit.h" 
 
int parse(const char *text, struct ast_node **node); 
 
#define COUNT 100
 
int test_cdir_split() 
{ 
    struct config* local_config = malloc(sizeof(struct config)); 
    local_config->lnode_max_cap = 3; 
    local_config->attr_domain_count = 1;
    local_config->attr_domains = malloc(sizeof(struct attr_domain));
    struct attr_domain attr_domain_a = { .name = "a", .minBound = 0, .maxBound = COUNT }; 
    local_config->attr_domains[0] = attr_domain_a; 

    const char* data[COUNT]; 

    for(unsigned int i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a = %d", i); 
        data[i] = expr; 
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(local_config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        insert_be_tree(local_config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event("a", 0); 
    struct matched_subs* matched_subs = make_matched_subs(); 
    match_be_tree(event, cnode, matched_subs); 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    uint64_t init_us = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t parse_us = (parse_done.tv_sec - init_done.tv_sec) * 1000000 + (parse_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - parse_done.tv_sec) * 1000000 + (insert_done.tv_nsec - parse_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000 + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Parse took %" PRIu64 "\n", parse_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

 
    for(unsigned int i = 0; i < COUNT; i++) { 
        free((char*)data[i]); 
    } 
    free(local_config->attr_domains); 
    free(local_config); 
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
} 

int test_pdir_split()
{
    struct config* local_config = malloc(sizeof(struct config)); 
    local_config->lnode_max_cap = 3; 
    local_config->attr_domain_count = COUNT; 
    local_config->attr_domains = malloc(COUNT * sizeof(struct attr_domain)); 

    const char* data[COUNT]; 

    for(unsigned int i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a%d = 0", i); 
        data[i] = expr; 

        char* name;
        asprintf(&name, "a%d", i);
        struct attr_domain attr_domain = { .name = name, .minBound = 0, .maxBound = 10 }; 
        local_config->attr_domains[i] = attr_domain; 
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(local_config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        insert_be_tree(local_config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event("a0", 0); 
    struct matched_subs* matched_subs = make_matched_subs(); 
    match_be_tree(event, cnode, matched_subs); 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

    uint64_t init_us = (init_done.tv_sec - start.tv_sec) * 1000000 + (init_done.tv_nsec - start.tv_nsec) / 1000;
    uint64_t parse_us = (parse_done.tv_sec - init_done.tv_sec) * 1000000 + (parse_done.tv_nsec - init_done.tv_nsec) / 1000;
    uint64_t insert_us = (insert_done.tv_sec - parse_done.tv_sec) * 1000000 + (insert_done.tv_nsec - parse_done.tv_nsec) / 1000;
    uint64_t search_us = (search_done.tv_sec - insert_done.tv_sec) * 1000000 + (search_done.tv_nsec - insert_done.tv_nsec) / 1000;

    printf("    Init took %" PRIu64 "\n", init_us);
    printf("    Parse took %" PRIu64 "\n", parse_us);
    printf("    Insert took %" PRIu64 "\n", insert_us);
    printf("    Search took %" PRIu64 "\n", search_us);

 
    for(unsigned int i = 0; i < COUNT; i++) { 
        free((char*)data[i]); 
    } 
    for(unsigned int i = 0; i < local_config->attr_domain_count; i++) {
        free((char*)local_config->attr_domains[i].name);
    }
    free(local_config->attr_domains); 
    free(local_config); 
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
}
 
int all_tests()  
{ 
    mu_run_test(test_cdir_split); 
    printf("\n");
    mu_run_test(test_pdir_split);
 
    return 0; 
} 
 
RUN_TESTS() 