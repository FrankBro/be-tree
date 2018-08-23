#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "tree.h"
#include "utils.h"
#include "value.h"

bool betree_delete(struct betree* betree, betree_sub_t id)
{
    struct sub* sub = find_sub_id(id, betree->cnode);
    bool found = betree_delete_inner(
        (const struct attr_domain**)betree->config->attr_domains, sub, betree->cnode);
    free_sub(sub);
    return found;
}

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct betree_event** event);

static bool is_valid(const struct config* config, const struct ast_node* node)
{
    bool var = all_variables_in_config(config, node);
    if(!var) {
        return false;
    }
    bool str = all_bounded_strings_valid(config, node);
    return str;
}


static void assign_indexes(struct sub* sub, struct config* config, size_t index_count, const struct betree_index** indexes)
{
    sub->index_count = calloc(index_count, sizeof(*sub->index_count));
    sub->indexes = calloc(index_count, sizeof(*sub->indexes));;
    for(size_t i = 0; i < index_count; i++) {
        const struct betree_index_definition* def = config->indexes[i];
        const struct betree_index* index = indexes[i];
        sub->index_count[i] = index->entries->entry_count;
        sub->indexes[i] = calloc(index->entries->entry_count, sizeof(*sub->indexes[i]));
        for(size_t j = 0; j < index->entries->entry_count; j++) {

        }
    }
}

static bool inject_indexes(struct config* config, size_t index_count, const struct betree_index** indexes, struct ast_node** output)
{
    for(size_t i = 0; i < index_count; i++) {
        const struct betree_index_definition* def = config->indexes[i];
        const struct betree_index* index = indexes[i];
        size_t entries_count = index->entries->entry_count;
        struct ast_index_entry** entries = calloc(entries_count, sizeof(*entries));
        for(size_t j = 0; j < entries_count; j++) {
            const struct betree_index_entry* entry = index->entries->entries[j];
            size_t var_count = entry->value_count;
            struct ast_index_var** vars = calloc(var_count, sizeof(*vars));
            for(size_t k = 0; i < entry->value_count; i++) {
                struct betree_index_variable* var_def = &def->variables[i];
                struct value* value = entry->values[i];
                struct index_value index_value;
                switch(value->value_type) {
                    case BETREE_STRING: {
                        index_value.value_type = AST_INDEX_VALUE_STRING;
                        index_value.string_value = value->svalue;
                        break;
                    }
                    case BETREE_INTEGER: {
                        index_value.value_type = AST_INDEX_VALUE_INTEGER;
                        index_value.integer_value = value->ivalue;
                        break;
                    }
                    case BETREE_FLOAT: {
                        index_value.value_type = AST_INDEX_VALUE_FLOAT;
                        index_value.float_value = value->fvalue;
                        break;
                    }
                    case BETREE_BOOLEAN:
                    case BETREE_INTEGER_LIST:
                    case BETREE_STRING_LIST:
                    case BETREE_SEGMENTS:
                    case BETREE_FREQUENCY_CAPS:
                    default:
                        fprintf(stderr, "invalid index value type\n");
                        abort();
                }
                struct ast_index_var* ast_var = ast_index_var_create(var_def->attr_var.attr, index_value);
                vars[k] = ast_var;
            }
            struct ast_index_entry* ast_entry = ast_index_entry_create(entry->id, var_count, vars);
            entries[j] = ast_entry;
        }
        struct ast_node* index_node = ast_index_expr_create(entries_count, entries);
        *output = ast_bool_expr_binary_create(AST_BOOL_AND, index_node, *output);
    }
    return true;
}

// index 1 : width, height
// index 2 : country
//
// expr : not private
// indexes 1 : [100, 200], [200, 300]
// indexes 2 : "us", "ca"
//
// expr: ((width = 100 && height = 200) || (width = 200 && height = 300))
//    && ((country = "us") || (country = "ca"))
//    && not private

bool betree_insert_with_struct(struct betree* tree, const struct betree_expression* expr)
{
    struct ast_node* node;
    if(parse(expr->expr, &node) != 0) {
        return false;
    }
    if(!is_valid(tree->config, node)) {
        free_ast_node(node);
        return false;
    }
    if(!inject_indexes(tree->config, expr->index_count, expr->indexes, &node)) {
        return false;
    }
    if(!assign_constants(expr->constant_count, expr->constants, node)) {
        return false;
    }
    assign_variable_id(tree->config, node);
    assign_str_id(tree->config, node);
    sort_lists(node);
    assign_pred_id(tree->config, node);
    struct sub* sub = make_sub(tree->config, expr->id, node);
    return insert_be_tree(tree->config, sub, tree->cnode, NULL);
}

bool betree_insert(struct betree* tree, betree_sub_t id, const char* sexpr)
{
    const struct betree_expression expr = {
        .id = id,
        .constant_count = 0,
        .constants = NULL,
        .index_count = 0,
        .indexes = NULL,
        .expr = sexpr
    };
    return betree_insert_with_struct(tree, &expr);
}

static const struct betree_variable** make_environment(size_t attr_domain_count, const struct betree_event* event)
{
    const struct betree_variable** preds = calloc(attr_domain_count, sizeof(*preds));
    for(size_t i = 0; i < event->variable_count; i++) {
        if(event->variables[i] != NULL) {
            preds[event->variables[i]->attr_var.var] = event->variables[i];
        }
    }
    return preds;
}

static void betree_search_with_event_filled(const struct betree* betree, struct betree_event* event, struct report* report)
{
    const struct betree_variable** variables
        = make_environment(betree->config->attr_domain_count, event);
    if(validate_variables(betree->config, variables) == false) {
        fprintf(stderr, "Failed to validate event\n");
        abort();
    }
    betree_search_with_preds(betree->config, variables, betree->cnode, report);
}

void betree_search(const struct betree* tree, const char* event_str, struct report* report)
{
    struct betree_event* event = make_event_from_string(tree, event_str);
    betree_search_with_event_filled(tree, event, report);
    free_event(event);
}

void betree_search_with_event(const struct betree* betree, struct betree_event* event, struct report* report)
{
    fill_event(betree->config, event);
    betree_search_with_event_filled(betree, event, report);
}

struct report* make_report()
{
    struct report* report = calloc(1, sizeof(*report));
    if(report == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    report->evaluated = 0;
    report->matched = 0;
    report->memoized = 0;
    report->shorted = 0;
    report->subs = NULL;
    return report;
}

void free_report(struct report* report)
{
    free(report->subs);
    free(report);
}

static struct betree* betree_make_with_config(struct config* config)
{
    struct betree* tree = calloc(1, sizeof(*tree));
    if(tree == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    tree->config = config;
    tree->cnode = make_cnode(tree->config, NULL);
    return tree;
}

struct betree* betree_make()
{
    struct config* config = make_default_config();
    return betree_make_with_config(config);
}

struct betree* betree_make_with_parameters(uint64_t lnode_max_cap, uint64_t min_partition_size)
{
    struct config* config = make_config(lnode_max_cap, min_partition_size);
    return betree_make_with_config(config);
}

void betree_free(struct betree* tree)
{
    free_cnode(tree->cnode);
    free_config(tree->config);
    free(tree);
}

void betree_add_boolean_variable(struct betree* betree, const char* name, bool allow_undefined)
{
    add_attr_domain_b(betree->config, name, allow_undefined);
}

void betree_add_integer_variable(
    struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max)
{
    add_attr_domain_bounded_i(betree->config, name, allow_undefined, min, max);
}

void betree_add_float_variable(
    struct betree* betree, const char* name, bool allow_undefined, double min, double max)
{
    add_attr_domain_bounded_f(betree->config, name, allow_undefined, min, max);
}

void betree_add_string_variable(
    struct betree* betree, const char* name, bool allow_undefined, size_t count)
{
    add_attr_domain_bounded_s(betree->config, name, allow_undefined, count);
}

void betree_add_integer_list_variable(
    struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max)
{
    add_attr_domain_bounded_il(betree->config, name, allow_undefined, min, max);
}

void betree_add_string_list_variable(
    struct betree* betree, const char* name, bool allow_undefined, size_t count)
{
    add_attr_domain_bounded_sl(betree->config, name, allow_undefined, count);
}

void betree_add_segments_variable(struct betree* betree, const char* name, bool allow_undefined)
{
    add_attr_domain_segments(betree->config, name, allow_undefined);
}

void betree_add_frequency_caps_variable(
    struct betree* betree, const char* name, bool allow_undefined)
{
    add_attr_domain_frequency(betree->config, name, allow_undefined);
}

struct betree_constant* betree_make_integer_constant(const char* name, int64_t ivalue)
{
    struct betree_constant* constant = malloc(sizeof(*constant));
    if(constant == NULL) {
        fprintf(stderr, "%s malloc failed", __func__);
        abort();
    }
    constant->name = strdup(name);
    struct value value = { .value_type = BETREE_INTEGER, .ivalue = ivalue };
    constant->value = value;
    return constant;
}

void betree_free_constant(struct betree_constant* constant)
{
    free_value(constant->value);
    free((char*)constant->name);
    free(constant);
}

void betree_free_constants(size_t count, struct betree_constant** constants)
{
    for(size_t i = 0; i < count; i++) {
        betree_free_constant(constants[i]);
    }
}

struct betree_integer_list* betree_make_integer_list(size_t count)
{
    struct betree_integer_list* list = malloc(sizeof(*list));
    list->count = count;
    list->integers = calloc(count, sizeof(*list->integers));
    return list;
}

void betree_add_integer(struct betree_integer_list* list, size_t index, int64_t value)
{
    list->integers[index] = value;
}

struct betree_string_list* betree_make_string_list(size_t count)
{
    struct betree_string_list* list = malloc(sizeof(*list));
    list->count = count;
    list->strings = calloc(count, sizeof(*list->strings));
    return list;
}

void betree_add_string(struct betree_string_list* list, size_t index, const char* value)
{
    struct string_value s = { .string = strdup(value) };
    list->strings[index] = s;
}

struct betree_segments* betree_make_segments(size_t count)
{
    struct betree_segments* segments = malloc(sizeof(*segments));
    segments->size = count;
    segments->content = calloc(count, sizeof(*segments->content));
    return segments;
}

struct betree_segment* betree_make_segment(int64_t id, int64_t timestamp)
{
    return make_segment(id, timestamp);
}

void betree_add_segment(
    struct betree_segments* segments, size_t index, struct betree_segment* segment)
{
    segments->content[index] = segment;
}


struct betree_frequency_caps* betree_make_frequency_caps(size_t count)
{
    struct betree_frequency_caps* frequency_caps = malloc(sizeof(*frequency_caps));
    frequency_caps->size = count;
    frequency_caps->content = calloc(count, sizeof(*frequency_caps->content));
    return frequency_caps;
}

struct betree_frequency_cap* betree_make_frequency_cap(const char* stype,
    uint32_t id,
    const char* ns,
    bool timestamp_defined,
    int64_t timestamp,
    uint32_t value)
{
    struct string_value namespace
        = { .string = strdup(ns), .str = INVALID_STR, .var = INVALID_VAR };
    return make_frequency_cap(stype, id, namespace, timestamp_defined, timestamp, value);
}

void betree_add_frequency_cap(struct betree_frequency_caps* frequency_caps,
    size_t index,
    struct betree_frequency_cap* frequency_cap)
{
    frequency_caps->content[index] = frequency_cap;
}

static struct betree_variable* betree_make_variable(const char* name, struct value value)
{
    struct attr_var attr_var = make_attr_var(name, NULL);
    struct betree_variable* var = malloc(sizeof(*var));
    var->attr_var = attr_var;
    var->value = value;
    return var;
}

struct betree_variable* betree_make_boolean_variable(const char* name, bool value)
{
    struct value v = { .value_type = BETREE_BOOLEAN, .bvalue = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_integer_variable(const char* name, int64_t value)
{
    struct value v = { .value_type = BETREE_INTEGER, .ivalue = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_float_variable(const char* name, double value)
{
    struct value v = { .value_type = BETREE_FLOAT, .fvalue = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_string_variable(const char* name, const char* value)
{
    struct string_value svalue = { .string = strdup(value), .var = INVALID_VAR, .str = INVALID_STR };
    struct value v = { .value_type = BETREE_STRING, .svalue = svalue };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_integer_list_variable(
    const char* name, struct betree_integer_list* value)
{
    struct value v = { .value_type = BETREE_INTEGER_LIST, .ilvalue = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_string_list_variable(
    const char* name, struct betree_string_list* value)
{
    struct value v = { .value_type = BETREE_STRING_LIST, .slvalue = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_segments_variable(
    const char* name, struct betree_segments* value)
{
    struct value v = { .value_type = BETREE_SEGMENTS, .segments_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_frequency_caps_variable(
    const char* name, struct betree_frequency_caps* value)
{
    struct value v = { .value_type = BETREE_FREQUENCY_CAPS, .frequency_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable_definition betree_get_variable_definition(struct betree* betree, size_t index)
{
    struct attr_domain* d = betree->config->attr_domains[index];
    struct betree_variable_definition def = { .name = d->attr_var.attr, .type = d->bound.value_type };
    return def;
}

void betree_free_variable(struct betree_variable* variable)
{
    free((char*)variable->attr_var.attr);
    free_value(variable->value);
    free(variable);
}

void betree_free_event(struct betree_event* event)
{
    free_event(event);
}

void betree_free_integer_list(struct betree_integer_list* value)
{
    free_integer_list(value);
}

void betree_free_string_list(struct betree_string_list* value)
{
    free_string_list(value);
}

void betree_free_segment(struct betree_segment* value)
{
    free_segment(value);
}

void betree_free_segments(struct betree_segments* value)
{
    free_segments(value);
}

void betree_free_frequency_cap(struct betree_frequency_cap* value)
{
    free_frequency_cap(value);
}

void betree_free_frequency_caps(struct betree_frequency_caps* value)
{
    free_frequency_caps(value);
}

struct betree_event* betree_make_event(const struct betree* betree)
{
    struct betree_event* event = malloc(sizeof(*event));
    event->variable_count = betree->config->attr_domain_count;
    event->variables = calloc(event->variable_count, sizeof(*event->variables));
    return event;
}

void betree_set_variable(struct betree_event* event, size_t index, struct betree_variable* variable)
{
    event->variables[index] = variable;
}

struct betree_index_definition* betree_make_index_definition(const char* name)
{
    struct betree_index_definition* index = malloc(sizeof(*index));
    index->name = strdup(name);
    index->variable_count = 0;
    index->variables = NULL;
    return index;
}

void betree_add_string_index(struct betree_index_definition* index, const char* name, bool strict)
{
    struct betree_index_variable v = { .attr_var = make_attr_var(name, NULL), .strict = strict };
    if(index->variable_count == 0) {
        index->variables = calloc(1, sizeof(*index->variables));
        if(index->variables == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_index_variable* variables = realloc(index->variables, sizeof(*variables) * (index->variable_count + 1));
        if(variables == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        index->variables = variables;
    }
    index->variables[index->variable_count] = v;
    index->variable_count++;
}

void betree_add_index(struct betree* betree, struct betree_index_definition* index)
{
    if(betree->config->index_count == 0) {
        betree->config->indexes = calloc(1, sizeof(*betree->config->indexes));
        if(betree->config->indexes == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_index_definition** indexes = realloc(betree->config->indexes, sizeof(*indexes) * (betree->config->index_count + 1));
        if(indexes == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        betree->config->indexes = indexes;
    }
    betree->config->indexes[betree->config->index_count] = index;
    betree->config->index_count++;
}

struct betree_index_entries* betree_make_index_entries()
{
    struct betree_index_entries* entries = calloc(1, sizeof(*entries));
    return entries;
}

struct betree_index_entry* betree_make_index_entry(betree_index_id id)
{
    struct betree_index_entry* entry = calloc(1, sizeof(*entry));
    entry->id = id;
    return entry;
}

static void betree_add_index_value(struct betree_index_entry* entry, struct value* value)
{
    if(entry->value_count == 0) {
        entry->values = calloc(1, sizeof(*entry->values));
        if(entry->values == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct value** values = realloc(entry->values, sizeof(*values) * (entry->value_count + 1));
        if(values == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        entry->values = values;
    }
    entry->values[entry->value_count] = value;
    entry->value_count++;
}

static struct value* make_value(enum betree_value_type_e value_type)
{
    struct value* value = malloc(sizeof(*value));
    value->value_type = value_type;
    return value;
}

void betree_add_index_string_value(struct betree_index_entry* entry, const char* svalue)
{
    struct value* value = make_value(BETREE_STRING);
    value->svalue.string = strdup(svalue);
    betree_add_index_value(entry, value);
}

void betree_add_index_undefined_value(struct betree_index_entry* entry)
{
    betree_add_index_value(entry, NULL);
}

void betree_add_index_entry(struct betree_index_entries* entries, struct betree_index_entry* entry)
{
    if(entries->entry_count == 0) {
        entries->entries = calloc(1, sizeof(*entries->entries));
        if(entries->entries == NULL) {
            fprintf(stderr, "%s calloc failed\n", __func__);
            abort();
        }
    }
    else {
        struct betree_index_entry** inner_entries = realloc(entries->entries, sizeof(*inner_entries) * (entries->entry_count + 1));
        if(inner_entries == NULL) {
            fprintf(stderr, "%s realloc failed\n", __func__);
            abort();
        }
        entries->entries = inner_entries;
    }
    entries->entries[entries->entry_count] = entry;
    entries->entry_count++;
}

struct betree_index* betree_make_index(const char* name, struct betree_index_entries* entries)
{
    struct betree_index* index = malloc(sizeof(*index));
    index->name = strdup(name);
    index->entries = entries;
    return index;
}

