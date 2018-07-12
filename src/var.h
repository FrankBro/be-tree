#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "value.h"

struct pred;

struct attr_var {
    const char* attr;
    betree_var_t var;
};

bool get_variable(betree_var_t var, const struct pred** preds, struct value* value);
bool get_float_var(betree_var_t var, const struct pred** preds, double* ret);
bool get_string_var(betree_var_t var, const struct pred** preds, struct string_value* ret);
bool get_integer_var(betree_var_t var, const struct pred** preds, int64_t* ret);
bool get_bool_var(betree_var_t var, const struct pred** preds, bool* ret);
bool get_integer_list_var(betree_var_t var, const struct pred** preds, struct integer_list_value* ret);
bool get_string_list_var(betree_var_t var, const struct pred** preds, struct string_list_value* ret);
bool get_segments_var(betree_var_t var, const struct pred** preds, struct segments_list* ret); 
bool get_frequency_var(betree_var_t var, const struct pred** preds, struct frequency_caps_list* ret);

bool is_empty_list(struct value value);

