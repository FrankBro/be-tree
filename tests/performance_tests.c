#include <time.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <inttypes.h>
 
#include "ast.h" 
#include "parser.h" 
#include "minunit.h" 
 
int parse(const char *text, struct ast_node **node); 
 
#define COUNT 100
 
int test_cdir_split() 
{
    struct config* config = make_default_config();
    add_attr_domain(config, "a", 0, COUNT);

    const char* data[COUNT]; 

    for(unsigned int i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a = %d", i); 
        data[i] = expr; 
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(config, i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event(config, "a", 0); 
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
    free(config->attr_domains); 
    free(config); 
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
} 

int test_pdir_split()
{
    struct config* config = make_default_config();

    const char* data[COUNT]; 

    for(unsigned int i = 0; i < COUNT; i++) { 
        char* expr; 
        asprintf(&expr, "a%d = 0", i); 
        data[i] = expr; 

        char* name;
        asprintf(&name, "a%d", i);
        add_attr_domain(config, name, 0, 10);
    } 
 
    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
    struct ast_node* node; 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    const struct sub* subs[COUNT];
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        if(parse(data[i], &node) != 0) { 
            return 1; 
        } 
        const struct sub* sub = make_sub(config, i + 1, node); 
        subs[i] = sub;
    } 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(unsigned int i = 0; i < COUNT; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event(config, "a0", 0); 
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
    free_config(config);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    return 0; 
}

#include <limits.h>
 
int test_complex()
{
    struct config* config = make_default_config();


    struct timespec start, init_done, parse_done, insert_done, search_done; 
 
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
 
    struct cnode* cnode = make_cnode(config, NULL); 
     
    clock_gettime(CLOCK_MONOTONIC_RAW, &init_done);

    FILE* f = fopen("output.be", "r");
    mu_assert(f != NULL, "can open output.be");

    char line[LINE_MAX];
    struct ast_node* node; 
    unsigned int sub_count = 0;
    struct sub** subs;
    while(fgets(line, sizeof(line), f)) {
        if(parse(line, &node) != 0) {
            printf("Failed to parse: %s\n", line);
            return 1;
        }
        adjust_attr_domains(config, node, 0, 100);
        const struct sub* sub = make_sub(config, sub_count + 1, node); 
        if(sub_count == 0) {
            subs = malloc(sizeof(struct sub*));
            if(subs == NULL) {
                return 1;
            }
        }
        else {
            struct sub** next_subs = realloc(subs, sizeof(struct sub*) * (sub_count + 1));
            if(next_subs == NULL) {
                return 1;
            }
            subs = next_subs;
        }
        subs[sub_count] = (struct sub*)sub;
        sub_count++;
    }

    fclose(f);

    clock_gettime(CLOCK_MONOTONIC_RAW, &parse_done);
 
    for(unsigned int i = 0; i < sub_count; i++) { 
        insert_be_tree(config, subs[i], cnode, NULL); 
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);

    const struct event* event = make_simple_event(config, "a0", 0); 
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