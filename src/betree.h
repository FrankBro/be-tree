#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t betree_sub_t;

struct betree;
struct event;

struct report {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    size_t shorted;
    betree_sub_t* subs;
};

/*
 * Initialization
 */
struct betree* betree_make();
struct betree* betree_make_with_parameters(uint64_t lnode_max_cap, uint64_t min_partition_size);

void betree_add_boolean_variable(struct betree* betree, const char* name, bool allow_undefined);
void betree_add_integer_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_float_variable(struct betree* betree, const char* name, bool allow_undefined, double min, double max);
void betree_add_string_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_integer_list_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_string_list_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_segments_variable(struct betree* betree, const char* name, bool allow_undefined);
void betree_add_frequency_caps_variable(struct betree* betree, const char* name, bool allow_undefined);

/*
 * Runtime
 */
//bool betree_insert_all(struct betree* tree, size_t count, const char** exprs);
bool betree_insert(struct betree* tree, betree_sub_t id, const char* expr);
void betree_search(const struct betree* betree, const char* event, struct report* report);
void betree_search_with_event(const struct betree* betree, const struct event* event, struct report* report);
bool betree_delete(struct betree* betree, betree_sub_t id);

struct report* make_report();
void free_report(struct report* report);

/*
 * Destruction
 */
void betree_free(struct betree* betree);
