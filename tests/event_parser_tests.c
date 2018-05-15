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

int all_tests()
{
    mu_run_test(test_bool);
    mu_run_test(test_integer);
    mu_run_test(test_float);
    return 0;
}

RUN_TESTS()
