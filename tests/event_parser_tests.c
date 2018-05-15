#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "minunit.h"
#include "parser.h"
#include "utils.h"

int event_parse(const char* text, struct event** event);

int test_bool()
{
    struct event* event;
    event_parse("{\"true\": true, \"false\": false}", &event);
    mu_assert(event->pred_count == 2 && strcmp(event->preds[0]->attr_var.attr, "true") == 0
            && event->preds[0]->value.value_type == VALUE_B && event->preds[0]->value.bvalue == true
            && strcmp(event->preds[1]->attr_var.attr, "false") == 0
            && event->preds[1]->value.value_type == VALUE_B
            && event->preds[1]->value.bvalue == false,
        "true and false");
    free_event(event);
    return 0;
}

int all_tests()
{
    mu_run_test(test_bool);
    return 0;
}

RUN_TESTS()
