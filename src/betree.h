#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t betree_sub_t;

struct config;
struct cnode;

struct betree {
    struct config* config;
    struct cnode* cnode;
};

struct report {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    size_t shorted;
    betree_sub_t* subs;
};

struct report_counting {
    size_t evaluated;
    size_t matched;
    size_t memoized;
    size_t shorted;
    betree_sub_t* subs;
    int node_count;
    int ops_count;
};

struct betree_sub;
struct betree_constant;
struct betree_variable;

struct betree_event {
    size_t variable_count;
    struct betree_variable** variables;
};

/*
 * Types
 */

enum betree_value_type_e {
    BETREE_BOOLEAN,
    BETREE_INTEGER,
    BETREE_FLOAT,
    BETREE_STRING,
    BETREE_INTEGER_LIST,
    BETREE_STRING_LIST,
    BETREE_SEGMENTS,
    BETREE_FREQUENCY_CAPS,
    BETREE_INTEGER_ENUM,
};

struct betree_integer_list;
struct betree_string_list;
struct betree_segment;
struct betree_segments;
struct betree_frequency_cap;
struct betree_frequency_caps;

struct betree_integer_list* betree_make_integer_list(size_t count);
void betree_add_integer(struct betree_integer_list* list, size_t index, int64_t value);

struct betree_string_list* betree_make_string_list(size_t count);
void betree_add_string(struct betree_string_list* list, size_t index, const char* value);

struct betree_segments* betree_make_segments(size_t count);
struct betree_segment* betree_make_segment(int64_t id, int64_t timestamp);
void betree_add_segment(struct betree_segments* segments, size_t index, struct betree_segment* segment);

struct betree_frequency_caps* betree_make_frequency_caps(size_t count);
struct betree_frequency_cap* betree_make_frequency_cap(const char* stype, uint32_t id, const char* ns, bool timestamp_defined, int64_t timestamp, uint32_t value);
void betree_add_frequency_cap(struct betree_frequency_caps* frequency_caps, size_t index, struct betree_frequency_cap* frequency_cap);

struct betree_variable_definition {
    const char* name;
    enum betree_value_type_e type;
};

const struct betree_variable** make_environment(size_t attr_domain_count, const struct betree_event* event);

/*
 * Initialization
 */
void betree_init(struct betree* betree);
struct betree* betree_make();
struct betree* betree_make_with_parameters(uint64_t lnode_max_cap, uint64_t min_partition_size);

void betree_add_boolean_variable(struct betree* betree, const char* name, bool allow_undefined);
void betree_add_integer_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_float_variable(struct betree* betree, const char* name, bool allow_undefined, double min, double max);
void betree_add_string_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_integer_list_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max);
void betree_add_integer_enum_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_string_list_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count);
void betree_add_segments_variable(struct betree* betree, const char* name, bool allow_undefined);
void betree_add_frequency_caps_variable(struct betree* betree, const char* name, bool allow_undefined);

bool betree_change_boundaries(struct betree* tree, const char* expr);

const struct betree_sub* betree_make_sub(struct betree* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr);
bool betree_insert_sub(struct betree* tree, const struct betree_sub* sub);

/*
 * Runtime
 */
//bool betree_insert_all(struct betree* tree, size_t count, const char** exprs);
struct betree_variable_definition betree_get_variable_definition(struct betree* betree, size_t index);

struct betree_constant* betree_make_integer_constant(const char* name, int64_t integer_value);

struct betree_variable* betree_make_boolean_variable(const char* name, bool value);
struct betree_variable* betree_make_integer_variable(const char* name, int64_t value);
struct betree_variable* betree_make_float_variable(const char* name, double value);
struct betree_variable* betree_make_string_variable(const char* name, const char* value);
struct betree_variable* betree_make_integer_list_variable(const char* name, struct betree_integer_list* value);
struct betree_variable* betree_make_string_list_variable(const char* name, struct betree_string_list* value);
struct betree_variable* betree_make_segments_variable(const char* name, struct betree_segments* value);
struct betree_variable* betree_make_frequency_caps_variable(const char* name, struct betree_frequency_caps* value);

struct betree_event* betree_make_event(const struct betree* betree);
void betree_set_variable(struct betree_event* event, size_t index, struct betree_variable* variable);

bool betree_insert(struct betree* tree, betree_sub_t id, const char* expr);
bool betree_insert_with_constants(struct betree* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr);

bool betree_search(const struct betree* tree, const char* event_str, struct report* report);
bool betree_search_ids(const struct betree* tree, const char* event_str, struct report* report, const uint64_t* ids, size_t sz);
bool betree_search_with_event(const struct betree* betree, struct betree_event* event, struct report* report);
bool betree_search_with_event_ids(const struct betree* betree, struct betree_event* event, struct report* report, const uint64_t* ids, size_t sz);

bool betree_exists(const struct betree* tree, const char* event_str);
bool betree_exists_with_event(const struct betree* betree, struct betree_event* event);

//bool betree_delete(struct betree* betree, betree_sub_t id);

struct report* make_report();
void free_report(struct report* report);

struct report_counting* make_report_counting();
void free_report_counting(struct report_counting* report);

/*
 * Destruction
 */
void betree_deinit(struct betree* betree);
void betree_free(struct betree* betree);

void betree_free_constant(struct betree_constant* constant);
void betree_free_constants(size_t count, struct betree_constant** constants);

void betree_free_variable(struct betree_variable* variable);
void betree_free_event(struct betree_event* event);

void betree_free_integer_list(struct betree_integer_list* value);
void betree_free_string_list(struct betree_string_list* value);
void betree_free_segment(struct betree_segment* value);
void betree_free_segments(struct betree_segments* value);
void betree_free_frequency_cap(struct betree_frequency_cap* value);
void betree_free_frequency_caps(struct betree_frequency_caps* value);

