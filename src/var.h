#pragma once

#include "tree.h"

enum variable_state_e {
    VARIABLE_DEFINED,
    VARIABLE_UNDEFINED,
    VARIABLE_MISSING,
};

enum variable_state_e get_variable(const struct config* config,
    betree_var_t variable_id,
    const struct event* event,
    struct value* value);

enum variable_state_e get_float_var(
    const struct config* config, betree_var_t var, const struct event* event, double* ret);
enum variable_state_e get_float_attr(
    const struct config* config, const struct event* event, const char* attr, double* ret);

enum variable_state_e get_string_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct string_value* ret);
enum variable_state_e get_string_attr(
    const struct config* config, const struct event* event, const char* attr, struct string_value* ret);

enum variable_state_e get_integer_var(
    const struct config* config, betree_var_t var, const struct event* event, int64_t* ret);
enum variable_state_e get_integer_attr(
    const struct config* config, const struct event* event, const char* attr, int64_t* ret);

enum variable_state_e get_bool_var(
    const struct config* config, betree_var_t var, const struct event* event, bool* ret);

enum variable_state_e get_integer_list_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct integer_list_value* ret);

enum variable_state_e get_string_list_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct string_list_value* ret);

enum variable_state_e get_segments_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct segments_list* ret);
enum variable_state_e get_segments_attr(
    const struct config* config, const struct event* event, const char* attr, struct segments_list* ret);

enum variable_state_e get_frequency_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct frequency_caps_list* ret);
enum variable_state_e get_frequency_attr(
    const struct config* config, const struct event* event, struct frequency_caps_list* ret);

bool is_empty_list(struct value value);
