#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <valgrind/callgrind.h>
#include <gsl/gsl_histogram.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_statistics.h>

#include "betree.h"
#include "debug.h"
#include "hashmap.h"
#include "utils.h"

#define MAX_EXPRS 5000
#define MAX_EVENTS 1000
#define DEFAULT_SEARCH_COUNT 10

struct events {
    size_t count;
    char** events;
};

extern bool MATCH_NODE_DEBUG;

struct value_bound get_integer_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .imin = INT64_MAX, .imax = INT64_MIN };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.ivalue < bound.imin) {
                    bound.imin = pred->value.ivalue;
                }
                if(pred->value.ivalue > bound.imax) {
                    bound.imax = pred->value.ivalue;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_integer_list_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .is_integer_list_bounded = true, .ilmin = INT64_MAX, .ilmax = INT64_MIN };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                for(size_t k = 0; k < pred->value.ilvalue.count; k++) {
                    int64_t value = pred->value.ilvalue.integers[k];
                    if(value < bound.ilmin) {
                        bound.ilmin = value;
                    }
                    if(value > bound.ilmax) {
                        bound.ilmax = value;
                    }
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_float_events_bound(betree_var_t var, struct event** events, size_t event_count) 
{
    struct value_bound bound = { .fmin = DBL_MAX, .fmax = -DBL_MAX };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.fvalue < bound.fmin) {
                    bound.fmin = pred->value.fvalue;
                }
                if(pred->value.fvalue > bound.fmax) {
                    bound.fmax = pred->value.fvalue;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_string_events_bound(betree_var_t var, struct event** events, size_t event_count)
{
    struct value_bound bound = { .is_string_bounded = true, .smin = -1, .smax = 0 };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                if(pred->value.svalue.str < bound.smin) {
                    bound.smin = pred->value.svalue.str;
                }
                if(pred->value.svalue.str > bound.smax && pred->value.svalue.str != UINT64_MAX) {
                    bound.smax = pred->value.svalue.str;
                }
                break;
            }
        }
    }
    return bound;
}

struct value_bound get_string_list_events_bound(betree_var_t var, struct event** events, size_t event_count)
{
    struct value_bound bound = { .is_string_list_bounded = true, .slmin = -1, .slmax = 0 };
    for(size_t i = 0; i < event_count; i++) {
        struct event* event = events[i];
        for(size_t j = 0; j < event->pred_count; j++) {
            struct pred* pred = event->preds[j];
            if(pred->attr_var.var == var) {
                for(size_t k = 0; k < pred->value.slvalue.count; k++) {
                    betree_str_t value = pred->value.slvalue.strings[k].str;
                    if(value < bound.smin) {
                        bound.slmin = value;
                    }
                    if(value > bound.smax && value != UINT64_MAX) {
                        bound.slmax = value;
                    }
                }
                break;
            }
        }
    }
    return bound;
}

void add_event(char* event, struct events* events)
{
    if(events->count == 0) {
        events->events = calloc(1, sizeof(*events->events));
        if(events == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        char** new_events
            = realloc(events->events, sizeof(*new_events) * ((events->count) + 1));
        if(new_events == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        events->events = new_events;
    }
    events->events[events->count] = strdup(event);
    events->count++;
}

char* strip_chars(const char* string, const char* chars)
{
    char* new_string = malloc(strlen(string) + 1);
    int counter = 0;

    for(; *string; string++) {
        if(!strchr(chars, *string)) {
            new_string[counter] = *string;
            ++counter;
        }
    }
    new_string[counter] = 0;
    return new_string;
}

int event_parse(const char* text, struct event** event);

size_t read_betree_events(struct config* config, struct events* events)
{
    FILE* f = fopen("betree_events", "r");
    size_t count = 0;

    char line[22000]; // Arbitrary from what I've seen
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EVENTS != 0 && events->count == MAX_EVENTS) {
            break;
        }

        struct event* event;
        if(event_parse(line, &event) != 0) {
            fprintf(stderr, "Can't parse event %zu: %s", events->count + 1, line);
            abort();
        }

        fill_event(config, event);
        if(!validate_event(config, event)) {
            abort();
        }
        add_event(line, events);
        count++;
    }
    fclose(f);
    return count;
}

size_t read_betree_exprs(struct betree* tree)
{
    FILE* f = fopen("betree_exprs", "r");

    //char* lines[MAX_EXPRS];
    char line[10000]; // Arbitrary from what I've seen
    size_t count = 0;
    while(fgets(line, sizeof(line), f)) {
        if(!betree_insert(tree, count, line)) {
            printf("Can't insert expr: %s\n", line);
            abort();
        }
        count++;
        if(MAX_EXPRS != 0 && count == MAX_EXPRS) {
            break;
        }
    }

    /*
    while(fgets(line, sizeof(line), f)) {
        lines[count] = strdup(line);
        count++;
        if(MAX_EXPRS != 0 && count == MAX_EXPRS) {
            break;
        }
    }

    betree_insert_all(tree, count, (const char**)lines);
    */

    fclose(f);
    return count;
}

void read_betree_defs(struct betree* tree)
{
    FILE* f = fopen("betree_defs", "r");

    char line[LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        betree_add_domain(tree, line);
    }

    fclose(f);
}

int compare_int( const void* a, const void* b )
{
    if( *(int*)a == *(int*)b ) return 0;
    return *(int*)a < *(int*)b ? -1 : 1;
}

struct attribute {
    betree_var_t var;
    size_t count;
};

struct operation {
    size_t attribute_count;
    struct attribute attributes[60]; // Can't be more than 60, for now
};

struct operations {
    struct operation set_in;
    struct operation set_not_in;
    struct operation list_one_of;
    struct operation list_none_of;
    struct operation list_all_of;
};

void add_attribute(betree_var_t var, struct operation* operation)
{
    for(size_t i = 0; i < operation->attribute_count; i++) {
        if(operation->attributes[i].var == var) {
            operation->attributes[i].count++;
            return;
        }
    }
    operation->attributes[operation->attribute_count].var = var;
    operation->attributes[operation->attribute_count].count = 1;
    operation->attribute_count++;
}

void extract_node(const struct ast_node* node, struct operations* operations)
{
    if(node->type == AST_TYPE_SET_EXPR) {
        if(node->set_expr.op == AST_SET_IN) {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                add_attribute(node->set_expr.left_value.variable_value.var, &operations->set_in);
            }
            else {
                add_attribute(node->set_expr.right_value.variable_value.var, &operations->set_in);
            }
        }
        if(node->set_expr.op == AST_SET_NOT_IN) {
            if(node->set_expr.left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
                add_attribute(node->set_expr.left_value.variable_value.var, &operations->set_not_in);
            }
            else {
                add_attribute(node->set_expr.right_value.variable_value.var, &operations->set_not_in);
            }
        }
    }
    else if(node->type == AST_TYPE_LIST_EXPR) {
        if(node->list_expr.op == AST_LIST_ONE_OF) {
            add_attribute(node->list_expr.attr_var.var, &operations->list_one_of);
        }
        else if(node->list_expr.op == AST_LIST_NONE_OF) {
            add_attribute(node->list_expr.attr_var.var, &operations->list_none_of);
        }
        else {
            add_attribute(node->list_expr.attr_var.var, &operations->list_all_of);
        }
    }
    else if(node->type == AST_TYPE_BOOL_EXPR) {
        if(node->bool_expr.op == AST_BOOL_NOT) {
            extract_node(node->bool_expr.unary.expr, operations);
        }
        else if(node->bool_expr.op == AST_BOOL_OR || node->bool_expr.op == AST_BOOL_AND) {
            extract_node(node->bool_expr.binary.lhs, operations);
            extract_node(node->bool_expr.binary.rhs, operations);
        }
    }
}

void extract_cnode(struct cnode* cnode, struct operations* operations);

void extract_cdir(struct cdir* cdir, struct operations* operations)
{
    extract_cnode(cdir->cnode, operations);
    if(cdir->lchild != NULL) {
        extract_cdir(cdir->lchild, operations);
    }
    if(cdir->rchild != NULL) {
        extract_cdir(cdir->rchild, operations);
    }
}

void extract_cnode(struct cnode* cnode, struct operations* operations)
{
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        extract_node(cnode->lnode->subs[i]->expr, operations);
    }
    if(cnode->pdir != NULL) {
        for(size_t i = 0; i < cnode->pdir->pnode_count; i++) {
            extract_cdir(cnode->pdir->pnodes[i]->cdir, operations);
        }
    }
}

void print_operation(struct config* config, const char* name, struct operation* operation)
{
    printf("%s: [", name);
    for(size_t i = 0; i < operation->attribute_count; i++) {
        if(i != 0) {
            printf(", ");
        }
        const char* attr = config->attr_domains[operation->attributes[i].var]->attr_var.attr;
        printf("%s: %zu", attr, operation->attributes[i].count);
    }
    printf("]\n");
}

void extract_operations(struct betree* tree)
{
    struct operations operations;
    operations.set_in.attribute_count = 0;
    operations.set_not_in.attribute_count = 0;
    operations.list_one_of.attribute_count = 0;
    operations.list_none_of.attribute_count = 0;
    operations.list_all_of.attribute_count = 0;
    extract_cnode(tree->cnode, &operations);
    print_operation(tree->config, "set_in", &operations.set_in);
    print_operation(tree->config, "set_not_in", &operations.set_not_in);
    print_operation(tree->config, "list_one_of", &operations.list_one_of);
    print_operation(tree->config, "list_none_of", &operations.list_none_of);
    print_operation(tree->config, "list_all_of", &operations.list_all_of);
}

int main(int argc, char** argv)
{
    size_t search_count = DEFAULT_SEARCH_COUNT;
    if(argc > 1) {
        search_count = atoi(argv[1]);
    }
    if(access("betree_defs", F_OK) == -1 || access("betree_events", F_OK) == -1
        || access("betree_exprs", F_OK) == -1) {
        fprintf(stderr, "Missing files, skipping the tests");
        return 0;
    }
    struct timespec start, insert_done, gen_event_done, search_done;

    // Init
    struct betree* tree = betree_make();
    read_betree_defs(tree);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Insert
    size_t expr_count = read_betree_exprs(tree);

    /*extract_operations(tree);*/

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);
    uint64_t insert_us = (insert_done.tv_sec - start.tv_sec) * 1000000
        + (insert_done.tv_nsec - start.tv_nsec) / 1000;
    printf("    Insert took %" PRIu64 "\n", insert_us);

    struct events events = { .count = 0, .events = NULL };
    size_t event_count = read_betree_events(tree->config, &events);

    uint64_t evaluated_sum = 0;
    uint64_t matched_sum = 0;
    uint64_t memoized_sum = 0;
    uint64_t shorted_sum = 0;

    const size_t search_us_count = search_count * events.count;
    double search_us_data[search_us_count];

    /*MATCH_NODE_DEBUG = true;*/

    /*FILE* fOut = fopen("real_test_output", "w");*/

    CALLGRIND_START_INSTRUMENTATION;

    size_t search_us_i = 0;
    
    for(size_t j = 0; j < search_count; j++) {
        for(size_t i = 0; i < events.count; i++) {
            clock_gettime(CLOCK_MONOTONIC_RAW, &gen_event_done);

            char* event = events.events[i];
            struct report* report = make_report();
            betree_search(tree, event, report);

            clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

            uint64_t search_us = (search_done.tv_sec - gen_event_done.tv_sec) * 1000000
                + (search_done.tv_nsec - gen_event_done.tv_nsec) / 1000;

            search_us_data[search_us_i] = (double)search_us;

            evaluated_sum += report->evaluated;
            matched_sum += report->matched;
            memoized_sum += report->memoized;
            shorted_sum += report->shorted;
            free_report(report);
            search_us_i++;
        }
        printf("Finished run %zu/%zu\n", j, search_count);
    }

    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;

    /*fclose(fOut);*/

    double evaluated_average = (double)evaluated_sum / (double)MAX_EVENTS;
    double matched_average = (double)matched_sum / (double)MAX_EVENTS;
    double memoized_average = (double)memoized_sum / (double)MAX_EVENTS;
    double shorted_average = (double)shorted_sum / (double)MAX_EVENTS;
    printf("%zu searches, %zu expressions, %zu events, %zu preds. Evaluated %.2f, matched %.2f, memoized %.2f, shorted %.2f\n",
        search_us_count,
        expr_count,
        event_count,
        tree->config->pred_map->pred_count,
        evaluated_average / search_count,
        matched_average / search_count,
        memoized_average / search_count,
        shorted_average / search_count);

    double search_us_min = gsl_stats_min(search_us_data, 1, search_us_count);
    double search_us_max = gsl_stats_max(search_us_data, 1, search_us_count);
    double search_us_mean = gsl_stats_mean(search_us_data, 1, search_us_count);
    gsl_sort(search_us_data, 1, search_us_count);
    double search_us_90 = gsl_stats_quantile_from_sorted_data(search_us_data, 1, search_us_count, 0.90);
    double search_us_95 = gsl_stats_quantile_from_sorted_data(search_us_data, 1, search_us_count, 0.95);
    double search_us_99 = gsl_stats_quantile_from_sorted_data(search_us_data, 1, search_us_count, 0.99);

    printf("Min: %.1f, Mean: %.1f, Max: %.1f, 90: %.1f, 95: %.1f, 99: %.1f\n", search_us_min, search_us_mean, search_us_max, search_us_90, search_us_95, search_us_99);

    printf("| %lu | %.1f | %.1f | %.1f | %.1f | %.1f | %.1f | |\n", insert_us, search_us_min, search_us_mean, search_us_max, search_us_90, search_us_95, search_us_99);

    // DEBUG
    write_dot_file(tree->config, tree->cnode);
    // DEBUG
    
    // <DEBUG>
    /*
    struct event* parsed_events[event_count];
    for(size_t i = 0; i < event_count; i++) {
        parsed_events[i] = make_event_from_string(tree->config, events.events[i]);
    }

    for(size_t i = 0; i < tree->config->attr_domain_count; i++) {
        const struct attr_domain* attr_domain = tree->config->attr_domains[i];
        if(attr_domain->bound.value_type == VALUE_I) {
            struct value_bound bound = get_integer_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            printf("    i, %s: [%ld, %ld]\n", attr_domain->attr_var.attr, bound.imin, bound.imax);
        }
        else if(attr_domain->bound.value_type == VALUE_F) {
            struct value_bound bound = get_float_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            printf("    f, %s: [%.2f, %.2f]\n", attr_domain->attr_var.attr, bound.fmin, bound.fmax);
        }
        else if(attr_domain->bound.value_type == VALUE_S) {
            struct value_bound bound = get_string_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            printf("    s, %s: %zu values\n", attr_domain->attr_var.attr, bound.smax);
        }
        else if(attr_domain->bound.value_type == VALUE_IL) {
            struct value_bound bound = get_integer_list_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            printf("    il, %s: [%ld, %ld]\n", attr_domain->attr_var.attr, bound.ilmin, bound.ilmax);
        }
        else if(attr_domain->bound.value_type == VALUE_SL) {
            struct value_bound bound = get_string_list_events_bound(attr_domain->attr_var.var, parsed_events, event_count);
            printf("    sl, %s: %zu values\n", attr_domain->attr_var.attr, bound.slmax);
        }
    }
    */ 
    // </DEBUG>

    free(events.events);
    betree_free(tree);
    return 0;
}

