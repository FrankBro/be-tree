#include <float.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "hashmap.h"
#include "memoize.h"
#include "utils.h"

struct config* make_config(uint64_t lnode_max_cap, uint64_t partition_min_size)
{
    struct config* config = calloc(1, sizeof(*config));
    if(config == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    config->abort_on_error = true;
    config->attr_domain_count = 0;
    config->attr_domains = NULL;
    config->attr_to_id_count = 0;
    config->attr_to_ids = NULL;
    config->lnode_max_cap = lnode_max_cap;
    config->partition_min_size = partition_min_size;
    config->max_domain_for_split = 1000;
    config->string_map_count = 0;
    config->string_maps = NULL;
    config->pred_map = make_pred_map();
    return config;
}

struct config* make_default_config()
{
    return make_config(3, 0);
}

void free_config(struct config* config)
{
    if(config == NULL) {
        return;
    }
    if(config->attr_to_ids != NULL) {
        for(size_t i = 0; i < config->attr_to_id_count; i++) {
            free(config->attr_to_ids[i]);
        }
        free(config->attr_to_ids);
        config->attr_to_ids = NULL;
    }
    if(config->attr_domains != NULL) {
        for(size_t i = 0; i < config->attr_domain_count; i++) {
            free((char*)config->attr_domains[i]->attr_var.attr);
            free(config->attr_domains[i]);
        }
        free(config->attr_domains);
        config->attr_domains = NULL;
    }
    if(config->string_maps != NULL) {
        for(size_t i = 0; i < config->string_map_count; i++) {
            free((char*)config->string_maps[i].attr_var.attr);
            for(size_t j = 0; j < config->string_maps[i].string_value_count; j++) {
                free(config->string_maps[i].string_values[j]);
            }
            free(config->string_maps[i].string_values);
        }
        free(config->string_maps);
        config->string_maps = NULL;
    }
    if(config->pred_map != NULL) {
        free_pred_map(config->pred_map);
        config->pred_map = NULL;
    }
    free(config);
}

static struct attr_domain* make_attr_domain(
    const char* attr, betree_var_t variable_id, struct value_bound bound, bool allow_undefined)
{
    struct attr_domain* attr_domain = calloc(1, sizeof(*attr_domain));
    if(attr_domain == NULL) {
        fprintf(stderr, "%s calloc faild\n", __func__);
        abort();
    }
    attr_domain->attr_var.attr = strdup(attr);
    attr_domain->attr_var.var = variable_id;
    attr_domain->bound = bound;
    attr_domain->allow_undefined = allow_undefined;
    return attr_domain;
}

static void add_attr_domain(
    struct config* config, const char* attr, struct value_bound bound, bool allow_undefined)
{
    betree_var_t variable_id = get_id_for_attr(config, attr);
    struct attr_domain* attr_domain = make_attr_domain(attr, variable_id, bound, allow_undefined);
    if(config->attr_domain_count == 0) {
        config->attr_domains = calloc(1, sizeof(*config->attr_domains));
        if(config->attr_domains == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct attr_domain** attr_domains = realloc(
            config->attr_domains, sizeof(*attr_domains) * (config->attr_domain_count + 1));
        if(attr_domains == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        config->attr_domains = attr_domains;
    }
    config->attr_domains[config->attr_domain_count] = attr_domain;
    config->attr_domain_count++;
}

void add_attr_domain_bounded_i(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max)
{
    struct value_bound bound = { .value_type = VALUE_I, .imin = min, .imax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_i(struct config* config, const char* attr, bool allow_undefined)
{
    add_attr_domain_bounded_i(config, attr, allow_undefined, INT64_MIN, INT64_MAX);
}

void add_attr_domain_bounded_f(struct config* config, const char* attr, bool allow_undefined, double min, double max)
{
    struct value_bound bound = { .value_type = VALUE_F, .fmin = min, .fmax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_f(struct config* config, const char* attr, bool allow_undefined)
{
    add_attr_domain_bounded_f(config, attr, allow_undefined, -DBL_MAX, DBL_MAX);
}

void add_attr_domain_b(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_B, .bmin = false, .bmax = true };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_s(struct config* config, const char* attr, bool allow_undefined)
{
    add_attr_domain_bounded_s(config, attr, allow_undefined, SIZE_MAX);
}

void add_attr_domain_bounded_s(struct config* config, const char* attr, bool allow_undefined, size_t max)
{
    struct value_bound bound = { .value_type = VALUE_S, .smin = 0, .smax = max - 1 };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_il(struct config* config, const char* attr, bool allow_undefined)
{
    add_attr_domain_bounded_il(config, attr, allow_undefined, INT64_MIN, INT64_MAX);
}

void add_attr_domain_bounded_il(struct config* config, const char* attr, bool allow_undefined, int64_t min, int64_t max)
{
    struct value_bound bound = { .value_type = VALUE_IL, .imin = min, .imax = max };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_sl(struct config* config, const char* attr, bool allow_undefined)
{
    add_attr_domain_bounded_sl(config, attr, allow_undefined, SIZE_MAX);
}

void add_attr_domain_bounded_sl(struct config* config, const char* attr, bool allow_undefined, size_t max)
{
    struct value_bound bound = { .value_type = VALUE_SL, .smin = 0, .smax = max - 1 };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_segments(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_SEGMENTS };
    add_attr_domain(config, attr, bound, allow_undefined);
}

void add_attr_domain_frequency(struct config* config, const char* attr, bool allow_undefined)
{
    struct value_bound bound = { .value_type = VALUE_FREQUENCY };
    add_attr_domain(config, attr, bound, allow_undefined);
}

const struct attr_domain* get_attr_domain(const struct config* config, betree_var_t variable_id)
{
    return config->attr_domains[variable_id];
}

static void add_string_map(struct attr_var attr_var, struct config* config)
{
    if(config->string_map_count == 0) {
        config->string_maps = calloc(1, sizeof(*config->string_maps));
        if(config->string_maps == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct string_map* string_maps = realloc(
            config->string_maps, sizeof(*string_maps) * (config->string_map_count + 1));
        if(string_maps == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        config->string_maps = string_maps;
    }
    config->string_maps[config->string_map_count].attr_var.attr = strdup(attr_var.attr);
    config->string_maps[config->string_map_count].attr_var.var = attr_var.var;
    config->string_maps[config->string_map_count].string_value_count = 0;
    config->string_maps[config->string_map_count].string_values = 0;
    config->string_map_count++;
}

static void add_to_string_map(struct string_map* string_map, char* copy)
{
    if(string_map->string_value_count == 0) {
        string_map->string_values = calloc(1, sizeof(*string_map->string_values));
        if(string_map->string_values == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        char** string_values = realloc(
            string_map->string_values, sizeof(*string_values) * (string_map->string_value_count + 1));
        if(string_values == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        string_map->string_values = string_values;
    }
    string_map->string_values[string_map->string_value_count] = copy;
    string_map->string_value_count++;
}

betree_str_t try_get_id_for_string(const struct config* config, struct attr_var attr_var, const char* string)
{
    char* copy = strdup(string);
    for(size_t i = 0; i < config->string_map_count; i++) {
        if(config->string_maps[i].attr_var.var == attr_var.var) {
            struct string_map* string_map = string_map = &config->string_maps[i];
            for(size_t j = 0; j < string_map->string_value_count; j++) {
                if(strcmp(string_map->string_values[j], copy) == 0) {
                    free(copy);
                    return j;
                }
            }
            break;
        }
    }
    free(copy);
    return UINT64_MAX;
}

betree_str_t get_id_for_string(struct config* config, struct attr_var attr_var, const char* string)
{
    char* copy = strdup(string);
    struct string_map* string_map = NULL;
    for(size_t i = 0; i < config->string_map_count; i++) {
        if(config->string_maps[i].attr_var.var == attr_var.var) {
            string_map = &config->string_maps[i];
            for(size_t j = 0; j < string_map->string_value_count; j++) {
                if(strcmp(string_map->string_values[j], copy) == 0) {
                    free(copy);
                    return j;
                }
            }
            break;
        }
    }
    if(string_map == NULL) {
        add_string_map(attr_var, config);
        string_map = &config->string_maps[config->string_map_count - 1];
    }
    const struct attr_domain* attr_domain = get_attr_domain(config, attr_var.var);
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_TYPE_MISMATCH, attr_domain != NULL && 
        (attr_domain->bound.value_type == VALUE_S || attr_domain->bound.value_type == VALUE_SL || attr_domain->bound.value_type == VALUE_FREQUENCY));
    if(attr_domain->bound.smax + 1 == string_map->string_value_count) {
        free(copy);
        return UINT64_MAX;
    }
    add_to_string_map(string_map, copy);
    return string_map->string_value_count - 1;
}

bool is_variable_allow_undefined(const struct config* config, const betree_var_t variable_id)
{
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_MISSING, variable_id < config->attr_domain_count);
    betree_assert(config->abort_on_error, ERROR_ATTR_DOMAIN_NOT_INDEX, config->attr_domains[variable_id]->attr_var.var == variable_id);
    return config->attr_domains[variable_id]->allow_undefined;
}

