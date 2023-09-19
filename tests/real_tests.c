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
#include "helper.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

#define MAX_EXPRS 10000
#define MAX_EVENTS 5000
#define DEFAULT_SEARCH_COUNT 10

// wc -L filename
#define MAX_EVENT_CHARACTERS 20000
#define MAX_EXPR_CHARACTERS 17000
#define MAX_CONSTANT_CHARACTERS 20

const char* EXPRS_FILE = "data/betree_exprs";
const char* CONSTANTS_FILE = "data/betree_constants";
const char* EVENTS_FILE = "data/betree_events";

struct betree_events {
    size_t count;
    char** events;
};


size_t max(size_t x, size_t y){
    if(x > y){
        return x;
    }else{
        return y;
    }
}

void add_event(char* event, struct betree_events* events)
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

int event_parse(const char* text, struct betree_event** event);

size_t read_betree_events(struct betree_events* events)
{
    FILE* f = fopen(EVENTS_FILE, "r");
    size_t count = 0;

    char line[MAX_EVENT_CHARACTERS]; // Arbitrary from what I've seen
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EVENTS != 0 && events->count == MAX_EVENTS) {
            break;
        }

        add_event(line, events);
        count++;
    }
    fclose(f);
    return count;
}

size_t read_betree_exprs(struct betree* tree)
{

    FILE* f = fopen(EXPRS_FILE, "r");
    FILE* constants_f = fopen(CONSTANTS_FILE, "r");

    //char* lines[MAX_EXPRS];
    char line[MAX_EXPR_CHARACTERS]; // Arbitrary from what I've seen
    char constants_line[MAX_CONSTANT_CHARACTERS];
    size_t count = 0;
    const struct betree_sub* subs[MAX_EXPRS];

    enum e { constant_count = 4 };
    while(fgets(line, sizeof(line), f)) {
        char* ignore = fgets(constants_line, sizeof(constants_line), constants_f);
        (void)ignore;
        char* copy = strdup(constants_line);
        char* rest = copy;
        int64_t boolean_expression_id = strtoll(strtok_r(rest, ",", &rest), NULL, 10);
        int64_t campaign_group_id = strtoll(strtok_r(rest, ",", &rest), NULL, 10);
        int64_t campaign_id = strtoll(strtok_r(rest, ",", &rest), NULL, 10);
        int64_t advertiser_id = strtoll(strtok_r(rest, ",", &rest), NULL, 10);
        int64_t flight_id = strtoll(strtok_r(rest, "\n", &rest), NULL, 10);
        const struct betree_constant* constants[constant_count] = {
            betree_make_integer_constant("campaign_group_id", campaign_group_id),
            betree_make_integer_constant("campaign_id", campaign_id),
            betree_make_integer_constant("advertiser_id", advertiser_id),
            betree_make_integer_constant("flight_id", flight_id),
        };

        const struct betree_sub* sub = betree_make_sub(tree, boolean_expression_id, constant_count, constants, line);
        subs[count] = sub;
        count++;
        betree_free_constants(constant_count, (struct betree_constant**) constants);
        free(copy);
        if(MAX_EXPRS != 0 && count == MAX_EXPRS) {
            break;
        }
    }
    /*for(size_t i = 0; i < tree->config->attr_domain_count; i++) {*/
        /*const struct attr_domain* attr_domain = tree->config->attr_domains[i];*/
        /*print_attr_domain(attr_domain);*/
    /*}*/
    fclose(f);
    fclose(constants_f);

    for(size_t i = 0; i < count; i++) {
        const struct betree_sub* sub = subs[i];
        if(!betree_insert_sub(tree, sub)) {
            printf("Can't insert expr %zu\n", i);
            abort();
        }
    }

    return count;
}

void read_betree_defs(struct betree* tree)
{
    FILE* f = fopen("data/betree_defs", "r");

    char line[LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        add_variable_from_string(tree, line);
    }

    fclose(f);
}

int compare_int( const void* a, const void* b )
{
    if( *(int*)a == *(int*)b ) return 0;
    return *(int*)a < *(int*)b ? -1 : 1;
}

int main(int argc, char** argv)
{
    size_t search_count = DEFAULT_SEARCH_COUNT;
    if(argc > 1) {
        search_count = atoi(argv[1]);
    }
    if(access("data/betree_defs", F_OK) == -1 || access("data/betree_events", F_OK) == -1
        || access("data/betree_exprs", F_OK) == -1 || access("data/betree_constants", F_OK) == -1) {
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

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);
    uint64_t insert_us = (insert_done.tv_sec - start.tv_sec) * 1000000
        + (insert_done.tv_nsec - start.tv_nsec) / 1000;
    printf("    Insert took %" PRIu64 "\n", insert_us);

    struct betree_events events = { .count = 0, .events = NULL };
    size_t event_count = read_betree_events(&events);

    uint64_t evaluated_sum = 0;
    uint64_t matched_sum = 0;
    uint64_t memoized_sum = 0;
    uint64_t shorted_sum = 0;

    const size_t search_us_count = search_count * events.count;
    double search_us_data[search_us_count];

    CALLGRIND_START_INSTRUMENTATION;

    size_t search_us_i = 0;
    size_t max_match_expression = 0;
    int zero_boolean_expression_match = 0;
    
    for(size_t j = 0; j < search_count; j++) {
        for(size_t i = 0; i < events.count; i++) {
            clock_gettime(CLOCK_MONOTONIC_RAW, &gen_event_done);

            char* event = events.events[i];
            struct report* report = make_report();
            if(betree_search(tree, event, report) == false) {
                fprintf(stderr, "Failed to search with event\n");
                abort();
            }

            clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);

            uint64_t search_us = (search_done.tv_sec - gen_event_done.tv_sec) * 1000000
                + (search_done.tv_nsec - gen_event_done.tv_nsec) / 1000;

            search_us_data[search_us_i] = (double)search_us;

            evaluated_sum += report->evaluated;
            matched_sum += report->matched;
            memoized_sum += report->memoized;
            shorted_sum += report->shorted;
            max_match_expression = max(max_match_expression, report->matched);
            zero_boolean_expression_match += report->matched == 0 ? 1:0;
            // print expression id match by boolean expression
            // printf("report subs id: [");
            // for(int ka=0;ka<report->matched;ka++){
            //     if(ka==0){
            //         printf("%ld", report->subs[ka]);
            //     }else{
            //         printf(",%ld", report->subs[ka]);
            //     }
            // }
            // printf("]\n");
            free_report(report);
            search_us_i++;
        }
        printf("Finished run %zu/%zu\n", j, search_count);
    }

    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;

    for(size_t i = 0; i < events.count; i++) {
        free(events.events[i]);
    }

    double evaluated_average = (double)evaluated_sum / (double)MAX_EVENTS;
    double matched_average = (double)matched_sum / (double)MAX_EVENTS;
    double memoized_average = (double)memoized_sum / (double)MAX_EVENTS;
    double shorted_average = (double)shorted_sum / (double)MAX_EVENTS;
    printf("%zu searches, %zu expressions, %zu events, %zu preds, %zu memoize preds. Evaluated %.2f, matched %.2f, memoized %.2f, shorted %.2f\n",
        search_us_count,
        expr_count,
        event_count,
        tree->config->pred_map->pred_count,
        tree->config->pred_map->memoize_count,
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
    printf("max expression matched: %ld\n", max_match_expression);
    printf("no  expression matched: %d\n", zero_boolean_expression_match);
    // DEBUG
    write_dot_file(tree);
    // DEBUG
    
    free(events.events);
    betree_free(tree);
    return 0;
}

