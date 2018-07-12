#include "betree.h"
#include "error.h"
#include "tree.h"
#include "utils.h"
#include "var.h"

bool get_variable(betree_var_t var, const struct pred** preds, struct value* value)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *value = pred->value;
        return true;
    }
    return false;
}

bool get_float_var(betree_var_t var, const struct pred** preds, double* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.fvalue;
        return true;
    }
    return false;
}

bool get_string_var(betree_var_t var, const struct pred** preds, struct string_value* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.svalue;
        return true;
    }
    return false;
}

bool get_integer_var(betree_var_t var, const struct pred** preds, int64_t* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.ivalue;
        return true;
    }
    return false;
}

bool get_bool_var(betree_var_t var, const struct pred** preds, bool* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.bvalue;
        return true;
    }
    return false;
}

bool get_integer_list_var(betree_var_t var, const struct pred** preds, struct integer_list_value* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.ilvalue;
        return true;
    }
    return false;
}

bool get_string_list_var(betree_var_t var, const struct pred** preds, struct string_list_value* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.slvalue;
        return true;
    }
    return false;
}

bool get_segments_var(betree_var_t var, const struct pred** preds, struct segments_list* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.segments_value;
        return true;
    }
    return false;
}

bool get_frequency_var(betree_var_t var, const struct pred** preds, struct frequency_caps_list* ret)
{
    const struct pred* pred = preds[var];
    if(pred != NULL) {
        *ret = pred->value.frequency_value;
        return true;
    }
    return false;
}

bool is_empty_list(struct value value)
{
    return (value.value_type == VALUE_IL || value.value_type == VALUE_SL
               || value.value_type == VALUE_SEGMENTS || value.value_type == VALUE_FREQUENCY)
        && value.ilvalue.count == 0 && value.slvalue.count == 0 && value.segments_value.size == 0
        && value.frequency_value.size == 0;
}

