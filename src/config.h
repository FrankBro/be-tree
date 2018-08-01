#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "config.h"
#include "var.h"

struct attr_domain {
    struct attr_var attr_var;
    struct value_bound bound;
    bool allow_undefined;
};

struct ast_node;
struct pred_map;

struct string_map {
    struct attr_var attr_var;
    struct {
        size_t string_value_count;
        char** string_values;
    };
};

struct config* make_config(uint8_t lnode_max_cap, uint8_t partition_min_size);
struct config* make_default_config();
void free_config(struct config* config);

struct config {
    uint8_t lnode_max_cap;
    uint8_t partition_min_size;
    uint32_t max_domain_for_split;
    struct {
        size_t attr_domain_count;
        struct attr_domain** attr_domains;
    };
    struct {
        size_t string_map_count;
        struct string_map* string_maps;
    };
    struct pred_map* pred_map;
};

void add_attr_domain_i(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_f(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_b(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined);
void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined);

void add_attr_domain_bounded_i(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max);
void add_attr_domain_bounded_f(struct config* config, const char* attr, bool allow_undefined, double min, double max);
void add_attr_domain_bounded_s(struct config* config, const char* attr, bool allow_undefined, size_t max);
void add_attr_domain_bounded_il(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max);
void add_attr_domain_bounded_sl(struct config* config, const char* attr, bool allow_undefined, size_t max);
const struct attr_domain* get_attr_domain(const struct attr_domain** attr_domains, betree_var_t variable_id);
bool is_variable_allow_undefined(const struct config* config, const betree_var_t variable_id);

const char* get_attr_for_id(const struct config* config, betree_var_t variable_id);
betree_var_t try_get_id_for_attr(const struct config* config, const char* attr);
betree_str_t try_get_id_for_string(const struct config* config, struct attr_var attr_var, const char* string);
betree_str_t get_id_for_string(struct config* config, struct attr_var attr_var, const char* string);

struct attr_var make_attr_var(const char* attr, struct config* config);
struct attr_var copy_attr_var(struct attr_var attr_var);
void free_attr_var(struct attr_var attr_var);

