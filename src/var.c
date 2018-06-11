#include "var.h"
#include "betree.h"
#include "error.h"
#include "utils.h"

enum variable_state_e get_variable(const struct config* config,
    betree_var_t variable_id,
    const struct event* event,
    struct value* value)
{
    for(size_t i = 0; i < event->pred_count; i++) {
        const struct pred* pred = event->preds[i];
        if(variable_id == pred->attr_var.var) {
            *value = pred->value;
            return VARIABLE_DEFINED;
        }
    }
    bool allow_undefined = is_variable_allow_undefined(config, variable_id);
    if(allow_undefined) {
        return VARIABLE_UNDEFINED;
    }
    else {
        return VARIABLE_MISSING;
    }
}

enum variable_state_e get_float_var(
    const struct config* config, betree_var_t var, const struct event* event, double* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, value.value_type == VALUE_F);
        *ret = value.fvalue;
    }
    return state;
}

enum variable_state_e get_float_attr(
    const struct config* config, const struct event* event, const char* attr, double* ret)
{
    betree_var_t var = try_get_id_for_attr(config, attr);
    return get_float_var(config, var, event, ret);
}

enum variable_state_e get_string_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct string_value* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, value.value_type == VALUE_S);
        *ret = value.svalue;
    }
    return state;
}

enum variable_state_e get_string_attr(
    const struct config* config, const struct event* event, const char* attr, struct string_value* ret)
{
    betree_var_t var = try_get_id_for_attr(config, attr);
    return get_string_var(config, var, event, ret);
}

enum variable_state_e get_integer_var(
    const struct config* config, betree_var_t var, const struct event* event, int64_t* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, value.value_type == VALUE_I);
        *ret = value.ivalue;
    }
    return state;
}

enum variable_state_e get_integer_attr(
    const struct config* config, const struct event* event, const char* attr, int64_t* ret)
{
    betree_var_t var = try_get_id_for_attr(config, attr);
    return get_integer_var(config, var, event, ret);
}

enum variable_state_e get_bool_var(
    const struct config* config, betree_var_t var, const struct event* event, bool* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, value.value_type == VALUE_B);
        *ret = value.bvalue;
    }
    return state;
}

enum variable_state_e get_integer_list_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct integer_list_value* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, 
            is_empty_list(value) || value.value_type == VALUE_IL);
        *ret = value.ilvalue;
    }
    return state;
}

enum variable_state_e get_string_list_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct string_list_value* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, 
            is_empty_list(value) || value.value_type == VALUE_SL);
        *ret = value.slvalue;
    }
    return state;
}

enum variable_state_e get_segments_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct segments_list* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, 
            is_empty_list(value) || value.value_type == VALUE_SEGMENTS);
        *ret = value.segments_value;
    }
    return state;
}

enum variable_state_e get_segments_attr(
    const struct config* config, const struct event* event, const char* attr, struct segments_list* ret)
{
    betree_var_t var = try_get_id_for_attr(config, attr);
    return get_segments_var(config, var, event, ret);
}

enum variable_state_e get_frequency_var(const struct config* config,
    betree_var_t var,
    const struct event* event,
    struct frequency_caps_list* ret)
{
    struct value value;
    enum variable_state_e state = get_variable(config, var, event, &value);
    if(state == VARIABLE_DEFINED) {
        betree_assert(config->abort_on_error, ERROR_VALUE_TYPE_MISMATCH, 
            is_empty_list(value) || value.value_type == VALUE_FREQUENCY);
        *ret = value.frequency_value;
    }
    return state;
}

enum variable_state_e get_frequency_attr(
    const struct config* config, const struct event* event, struct frequency_caps_list* ret)
{
    betree_var_t var = try_get_id_for_attr(config, "frequency_caps");
    return get_frequency_var(config, var, event, ret);
}

bool is_empty_list(struct value value)
{
    return (value.value_type == VALUE_IL || value.value_type == VALUE_SL
               || value.value_type == VALUE_SEGMENTS || value.value_type == VALUE_FREQUENCY)
        && value.ilvalue.count == 0 && value.slvalue.count == 0 && value.segments_value.size == 0
        && value.frequency_value.size == 0;
}
