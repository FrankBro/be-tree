#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "betree.h"
#include "debug.h"
#include "minunit.h"
#include "utils.h"

#define MAX_EXPRS 5000
#define MAX_EVENTS 1000

struct events {
    size_t count;
    struct event** events;
};

void add_event(struct event* event, struct events* events)
{
    if(events->count == 0) {
        events->events = calloc(1, sizeof(*events->events));
        if(events == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct event** new_events
            = realloc(events->events, sizeof(*new_events) * ((events->count) + 1));
        if(new_events == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        events->events = new_events;
    }
    events->events[events->count] = event;
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

struct integer_list_value string_to_integer_list_value(char* str)
{
    char* token;
    char* rest = str;
    struct integer_list_value list = { .count = 0, .integers = NULL };
    while((token = strtok_r(rest, ",", &rest))) {
        int64_t value = strtoll(token, NULL, 10);
        add_integer_list_value(value, &list);
    }
    return list;
}

struct string_list_value string_to_string_list_value(struct config* config, char* str)
{
    char* token;
    char* rest = str;
    struct string_list_value list = { .count = 0, .strings = NULL };
    while((token = strtok_r(rest, ",", &rest))) {
        struct string_value value = { .string = token, .str = get_id_for_string(config, token) };
        add_string_list_value(value, &list);
    }
    return list;
}

int event_parse(const char* text, struct event** event);

size_t read_betree_events(struct config* config, struct events* events)
{
    FILE* f = fopen("betree_events", "r");
    size_t count = 0;

    char line[LINE_MAX * 8];
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EVENTS != 0 && events->count == MAX_EVENTS - 1) {
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
        add_event(event, events);
        count++;
    }
    fclose(f);
    return count;
}

size_t read_betree_exprs(struct config* config, struct cnode* cnode)
{
    FILE* f = fopen("betree_exprs", "r");

    char line[LINE_MAX * 2];
    betree_sub_t sub_id = 0;
    while(fgets(line, sizeof(line), f)) {
        if(MAX_EXPRS != 0 && sub_id == MAX_EXPRS - 1) {
            break;
        }
        betree_insert(config, sub_id, line, cnode);
        sub_id++;
    }

    fclose(f);
    return sub_id + 1;
}

void read_betree_defs(struct config* config)
{
    FILE* f = fopen("betree_defs", "r");

    char line[LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        char* line_rest = line;
        const char* name = strtok_r(line_rest, "|", &line_rest);
        const char* type = strtok_r(line_rest, "|", &line_rest);
        bool allow_undefined = strcmp(strtok_r(line_rest, "|\n", &line_rest), "true") == 0;
        const char* min_str = strtok_r(line_rest, "|\n", &line_rest);
        const char* max_str = strtok_r(line_rest, "|\n", &line_rest);
        if(strcmp(type, "integer") == 0) {
            int64_t min = INT64_MIN, max = INT64_MAX;
            if(min_str != NULL) {
                min = strtoll(min_str, NULL, 10);
            }
            if(max_str != NULL) {
                max = strtoll(max_str, NULL, 10);
            }
            add_attr_domain_i(config, name, min, max, allow_undefined);
        }
        else if(strcmp(type, "float") == 0) {
            double min = -DBL_MAX, max = DBL_MAX;
            if(min_str != NULL) {
                min = atof(min_str);
            }
            if(max_str != NULL) {
                max = atof(max_str);
            }
            add_attr_domain_i(config, name, min, max, allow_undefined);
        }
        else if(strcmp(type, "boolean") == 0) {
            add_attr_domain_b(config, name, false, true, allow_undefined);
        }
        else if(strcmp(type, "frequency") == 0) {
            add_attr_domain_frequency(config, name, allow_undefined);
        }
        else if(strcmp(type, "segments") == 0) {
            add_attr_domain_segments(config, name, allow_undefined);
        }
        else if(strcmp(type, "string") == 0) {
            add_attr_domain_s(config, name, allow_undefined);
        }
        else if(strcmp(type, "integer list") == 0) {
            add_attr_domain_il(config, name, allow_undefined);
        }
        else if(strcmp(type, "string list") == 0) {
            add_attr_domain_sl(config, name, allow_undefined);
        }
        else {
            fprintf(stderr, "Unknown definition type");
            abort();
        }
    }

    fclose(f);
}

int test_real()
{
    if(access("betree_defs", F_OK) == -1 || access("betree_events", F_OK) == -1
        || access("betree_exprs", F_OK) == -1) {
        fprintf(stderr, "Missing files, skipping the tests");
        return 0;
    }
    struct timespec start, insert_done, gen_event_done, search_done;

    // Init
    struct config* config = make_default_config();
    read_betree_defs(config);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Insert
    struct cnode* cnode = make_cnode(config, NULL);
    size_t expr_count = read_betree_exprs(config, cnode);

    clock_gettime(CLOCK_MONOTONIC_RAW, &insert_done);
    uint64_t insert_us = (insert_done.tv_sec - start.tv_sec) * 1000000
        + (insert_done.tv_nsec - start.tv_nsec) / 1000;
    printf("    Insert took %" PRIu64 "\n", insert_us);

    struct events events = { .count = 0, .events = NULL };
    size_t event_count = read_betree_events(config, &events);

    uint64_t search_timings[MAX_EVENTS] = { 0 };
    uint64_t sum = 0;

    for(size_t i = 0; i < events.count; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &gen_event_done);

        struct event* event = events.events[i];
        struct matched_subs* matched_subs = make_matched_subs();
        struct report report = { .expressions_evaluated = 0, .expressions_matched = 0 };
        match_be_tree(config, event, cnode, matched_subs, &report);

        clock_gettime(CLOCK_MONOTONIC_RAW, &search_done);
        uint64_t search_us = (search_done.tv_sec - gen_event_done.tv_sec) * 1000000
            + (search_done.tv_nsec - gen_event_done.tv_nsec) / 1000;
        printf("    %05zu: Search took %" PRIu64 ", evaluated %zu expressions, matched %zu\n",
            i,
            search_us,
            report.expressions_evaluated,
            report.expressions_matched);
        search_timings[i] = search_us;
        sum += search_us;
        free_matched_subs(matched_subs);
        free_event((struct event*)event);
    }

    (void)search_timings;

    double average = (double)sum / (double)MAX_EVENTS;
    printf("For %zu expressions, %zu events, we have an average match time of %f us\n",
        expr_count,
        event_count,
        average);
    // DEBUG
    write_dot_file(config, cnode);
    // DEBUG


    free(events.events);
    free_cnode(cnode);
    free_config(config);
    return 0;
}

int all_tests()
{
    mu_run_test(test_real);

    return 0;
}

RUN_TESTS()
