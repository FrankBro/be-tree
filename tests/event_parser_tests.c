#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "minunit.h"
#include "utils.h"

int event_parse(const char* text, struct event** event);

bool test_bool_pred(const char* attr, bool value, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    return strcmp(pred->attr_var.attr, attr) == 0 && pred->value.value_type == VALUE_B
        && pred->value.bvalue == value;
}

bool test_integer_pred(const char* attr, int64_t value, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    return strcmp(pred->attr_var.attr, attr) == 0 && pred->value.value_type == VALUE_I
        && pred->value.ivalue == value;
}

bool test_float_pred(const char* attr, double value, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    return strcmp(pred->attr_var.attr, attr) == 0 && pred->value.value_type == VALUE_F
        && feq(pred->value.fvalue, value);
}

bool test_string_pred(const char* attr, const char* value, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    return strcmp(pred->attr_var.attr, attr) == 0 && pred->value.value_type == VALUE_S
        && strcmp(pred->value.svalue.string, value) == 0;
}

bool test_integer_list_pred(
    const char* attr, struct integer_list_value list, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    if(strcmp(pred->attr_var.attr, attr) == 0 && pred->value.value_type == VALUE_IL) {
        if(list.count == pred->value.ilvalue.count) {
            for(size_t i = 0; i < list.count; i++) {
                if(list.integers[i] != pred->value.ilvalue.integers[i]) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool test_integer_list_pred0(const char* attr, const struct event* event, size_t index)
{
    struct integer_list_value list = { .count = 0, .integers = NULL };
    return test_integer_list_pred(attr, list, event, index);
}

bool test_integer_list_pred1(const char* attr, int64_t i1, const struct event* event, size_t index)
{
    struct integer_list_value list = { .count = 0, .integers = NULL };
    add_integer_list_value(i1, &list);
    return test_integer_list_pred(attr, list, event, index);
}

bool test_integer_list_pred2(
    const char* attr, int64_t i1, int64_t i2, const struct event* event, size_t index)
{
    struct integer_list_value list = { .count = 0, .integers = NULL };
    add_integer_list_value(i1, &list);
    add_integer_list_value(i2, &list);
    return test_integer_list_pred(attr, list, event, index);
}

int test_bool()
{
    struct event* event;
    event_parse("{\"true\": true, \"false\": false}", &event);
    mu_assert(event->pred_count == 2 && test_bool_pred("true", true, event, 0)
            && test_bool_pred("false", false, event, 1),
        "true and false");
    free_event(event);
    return 0;
}

int test_integer()
{
    struct event* event;
    event_parse("{\"positive\": 1, \"negative\": -1}", &event);
    mu_assert(event->pred_count == 2 && test_integer_pred("positive", 1, event, 0)
            && test_integer_pred("negative", -1, event, 1),
        "positive and negative");
    free_event(event);
    return 0;
}

int test_float()
{
    struct event* event;
    event_parse("{\"no decimal\": 1., \"decimal\": 1.2, \"negative\": -1.}", &event);
    mu_assert(event->pred_count == 3 && test_float_pred("no decimal", 1., event, 0)
            && test_float_pred("decimal", 1.2, event, 1)
            && test_float_pred("negative", -1., event, 2),
        "all cases");
    free_event(event);
    return 0;
}

int test_string()
{
    struct event* event;
    event_parse("{\"normal\": \"normal\", \"empty\": \"\"}", &event);
    mu_assert(event->pred_count == 2 && test_string_pred("normal", "normal", event, 0)
            && test_string_pred("empty", "", event, 1),
        "empty or not");
    free_event(event);
    return 0;
}

int test_integer_list()
{
    struct event* event;
    event_parse("{\"single\":[1], \"two\":[1,2], \"empty\":[]}", &event);
    mu_assert(event->pred_count == 3 && test_integer_list_pred1("single", 1, event, 0)
            && test_integer_list_pred2("two", 1, 2, event, 1)
            && test_integer_list_pred0("empty", event, 2),
        "all cases");
    free_event(event);
    return 0;
}

int all_tests()
{
    mu_run_test(test_bool);
    mu_run_test(test_integer);
    mu_run_test(test_float);
    mu_run_test(test_string);
    mu_run_test(test_integer_list);
    // | string_list_value
    // | segments_value
    // | frequencies_value
    return 0;
}

RUN_TESTS()
