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

bool test_empty_list(const struct pred* pred)
{
    return (pred->value.value_type == VALUE_IL || pred->value.value_type == VALUE_SL
               || pred->value.value_type == VALUE_SEGMENTS
               || pred->value.value_type == VALUE_FREQUENCY)
        && pred->value.ilvalue.count == 0 && pred->value.slvalue.count == 0
        && pred->value.segments_value.size == 0 && pred->value.frequency_value.size == 0;
}

bool test_integer_list_pred(
    const char* attr, struct integer_list_value list, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    if(strcmp(pred->attr_var.attr, attr) == 0
        && (test_empty_list(pred) || pred->value.value_type == VALUE_IL)) {
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

bool test_string_list_pred(
    const char* attr, struct string_list_value list, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    if(strcmp(pred->attr_var.attr, attr) == 0
        && (test_empty_list(pred) || pred->value.value_type == VALUE_SL)) {
        if(list.count == pred->value.slvalue.count) {
            for(size_t i = 0; i < list.count; i++) {
                if(strcmp(list.strings[i].string, pred->value.slvalue.strings[i].string) != 0) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool test_string_list_pred0(const char* attr, const struct event* event, size_t index)
{
    struct string_list_value list = { .count = 0, .strings = NULL };
    return test_string_list_pred(attr, list, event, index);
}

bool test_string_list_pred1(
    const char* attr, const char* s1, const struct event* event, size_t index)
{
    struct string_list_value list = { .count = 0, .strings = NULL };
    struct string_value sv1 = { .string = s1 };
    add_string_list_value(sv1, &list);
    return test_string_list_pred(attr, list, event, index);
}

bool test_string_list_pred2(
    const char* attr, const char* s1, const char* s2, const struct event* event, size_t index)
{
    struct string_list_value list = { .count = 0, .strings = NULL };
    struct string_value sv1 = { .string = s1 };
    struct string_value sv2 = { .string = s2 };
    add_string_list_value(sv1, &list);
    add_string_list_value(sv2, &list);
    return test_string_list_pred(attr, list, event, index);
}

bool test_segment_list_pred(
    const char* attr, struct segments_list list, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    if(strcmp(pred->attr_var.attr, attr) == 0
        && (test_empty_list(pred) || pred->value.value_type == VALUE_SEGMENTS)) {
        if(list.size == pred->value.segments_value.size) {
            for(size_t i = 0; i < list.size; i++) {
                struct segment target = list.content[i];
                struct segment value = pred->value.segments_value.content[i];
                if(target.id != value.id || target.timestamp != value.timestamp) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool test_segment_list_pred0(const char* attr, const struct event* event, size_t index)
{
    struct segments_list list = { .size = 0, .content = NULL };
    return test_segment_list_pred(attr, list, event, index);
}

bool test_segment_list_pred1(
    const char* attr, int64_t id1, int64_t timestamp1, const struct event* event, size_t index)
{
    struct segments_list list = { .size = 0, .content = NULL };
    struct segment s1 = make_segment(id1, timestamp1);
    add_segment(s1, &list);
    return test_segment_list_pred(attr, list, event, index);
}

bool test_segment_list_pred2(const char* attr,
    int64_t id1,
    int64_t timestamp1,
    int64_t id2,
    int64_t timestamp2,
    const struct event* event,
    size_t index)
{
    struct segments_list list = { .size = 0, .content = NULL };
    struct segment s1 = make_segment(id1, timestamp1);
    struct segment s2 = make_segment(id2, timestamp2);
    add_segment(s1, &list);
    add_segment(s2, &list);
    return test_segment_list_pred(attr, list, event, index);
}

bool test_frequency_list_pred(
    const char* attr, struct frequency_caps_list list, const struct event* event, size_t index)
{
    const struct pred* pred = event->preds[index];
    if(strcmp(pred->attr_var.attr, attr) == 0
        && (test_empty_list(pred) || pred->value.value_type == VALUE_FREQUENCY)) {
        if(list.size == pred->value.frequency_value.size) {
            for(size_t i = 0; i < list.size; i++) {
                struct frequency_cap target = list.content[i];
                struct frequency_cap value = pred->value.frequency_value.content[i];
                if(target.id != value.id || target.timestamp != value.timestamp
                    || target.timestamp_defined != value.timestamp_defined
                    || target.type != value.type || target.value != value.value
                    || strcmp(target.namespace.string, value.namespace.string) != 0) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool test_frequency_list_pred0(const char* attr, const struct event* event, size_t index)
{
    struct frequency_caps_list list = { .size = 0, .content = NULL };
    return test_frequency_list_pred(attr, list, event, index);
}

bool test_frequency_list_pred1(const char* attr,
    const char* type1,
    int64_t id1,
    const char* namespace1,
    int64_t timestamp1,
    int64_t value1,
    const struct event* event,
    size_t index)
{
    struct frequency_caps_list list = { .size = 0, .content = NULL };
    struct string_value ns1 = { .string = namespace1 };
    struct frequency_cap s1 = make_frequency_cap(type1, id1, ns1, timestamp1, value1);
    add_frequency(s1, &list);
    return test_frequency_list_pred(attr, list, event, index);
}

bool test_frequency_list_pred2(const char* attr,
    const char* type1,
    int64_t id1,
    const char* namespace1,
    int64_t timestamp1,
    int64_t value1,
    const char* type2,
    int64_t id2,
    const char* namespace2,
    int64_t timestamp2,
    int64_t value2,
    const struct event* event,
    size_t index)
{
    struct frequency_caps_list list = { .size = 0, .content = NULL };
    struct string_value ns1 = { .string = namespace1 };
    struct frequency_cap s1 = make_frequency_cap(type1, id1, ns1, timestamp1, value1);
    struct string_value ns2 = { .string = namespace2 };
    struct frequency_cap s2 = make_frequency_cap(type2, id2, ns2, timestamp2, value2);
    add_frequency(s1, &list);
    add_frequency(s2, &list);
    return test_frequency_list_pred(attr, list, event, index);
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

int test_string_list()
{
    struct event* event;
    event_parse("{\"single\":[\"1\"], \"two\":[\"1\",\"2\"], \"empty\":[]}", &event);
    mu_assert(event->pred_count == 3 && test_string_list_pred1("single", "1", event, 0)
            && test_string_list_pred2("two", "1", "2", event, 1)
            && test_string_list_pred0("empty", event, 2),
        "all cases");
    free_event(event);
    return 0;
}

int test_segment()
{
    struct event* event;
    event_parse("{\"single\":[[1,2]], \"two\": [[1,2],[3,4]], \"empty\":[]}", &event);
    mu_assert(event->pred_count == 3 && test_segment_list_pred1("single", 1, 2, event, 0)
            && test_segment_list_pred2("two", 1, 2, 3, 4, event, 1)
            && test_segment_list_pred0("empty", event, 2),
        "all cases");
    free_event(event);
    return 0;
}

int test_frequency()
{
    struct event* event;
    event_parse("{\"single\":[[\"flight\",1,\"ns\",2,3]], \"two\": "
                "[[\"flight\",1,\"ns1\",2,3],[\"flight\",4,\"ns2\",5,6]], \"empty\":[]}",
        &event);
    mu_assert(event->pred_count == 3
            && test_frequency_list_pred1("single", "flight", 1, "ns", 2, 3, event, 0)
            && test_frequency_list_pred2("two",
                   "flight",
                   1,
                   "ns1",
                   2,
                   3,
                   "flight",
                   4,
                   "ns2",
                   5,
                   6,
                   event,
                   1)
            && test_frequency_list_pred0("empty", event, 2),
        "all cases");
    free_event(event);
    // event_parse("{\"test:test\": [[\"advertiser:ip\",0,\"\",0,0]]}", &event);
    // event_parse("{\"advertiser\": [[\"advertiser\",0,\"\",0,0]]}", &event);
    // mu_assert(event->pred_count == 1
    //         && test_frequency_list_pred1(
    //                "test:test", FREQUENCY_TYPE_ADVERTISER, 0, "", 0, 0, event, 0),
    //     "colon");
    event_parse("{\"advertiser\": [[\"advertiser\",0,\"\",0,0]],"
                "\"advertiser:ip\": [[\"advertiser:ip\",0,\"\",0,0]],"
                "\"campaign\": [[\"campaign\",0,\"\",0,0]],"
                "\"campaign:ip\": [[\"campaign:ip\",0,\"\",0,0]],"
                "\"flight\": [[\"flight\",0,\"\",0,0]],"
                "\"flight:ip\": [[\"flight:ip\",0,\"\",0,0]],"
                "\"product\": [[\"product\",0,\"\",0,0]],"
                "\"product:ip\": [[\"product:ip\",0,\"\",0,0]]}",
        &event);
    mu_assert(event->pred_count == 8
            && test_frequency_list_pred1(
                   "advertiser", "advertiser", 0, "", 0, 0, event, 0)
            && test_frequency_list_pred1(
                   "advertiser:ip", "advertiser:ip", 0, "", 0, 0, event, 1)
            && test_frequency_list_pred1(
                   "campaign", "campaign", 0, "", 0, 0, event, 2)
            && test_frequency_list_pred1(
                   "campaign:ip", "campaign:ip", 0, "", 0, 0, event, 3)
            && test_frequency_list_pred1(
                   "flight", "flight", 0, "", 0, 0, event, 4)
            && test_frequency_list_pred1(
                   "flight:ip", "flight:ip", 0, "", 0, 0, event, 5)
            && test_frequency_list_pred1(
                   "product", "product", 0, "", 0, 0, event, 6)
            && test_frequency_list_pred1(
                   "product:ip", "product:ip", 0, "", 0, 0, event, 7),
        "all types");
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
    mu_run_test(test_string_list);
    mu_run_test(test_segment);
    mu_run_test(test_frequency);
    return 0;
}

RUN_TESTS()
