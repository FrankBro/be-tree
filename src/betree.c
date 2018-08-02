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

bool betree_delete(struct betree* betree, betree_sub_t id)
{
    struct sub* sub = find_sub_id(id, betree->cnode);
    bool found = betree_delete_inner((const struct attr_domain**)betree->config->attr_domains, sub, betree->cnode);
    free_sub(sub);
    return found;
}

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct event** event);

static bool is_valid(const struct config* config, const struct ast_node* node)
{
    bool var = all_variables_in_config(config, node);
    if(!var) {
        return false;
    }
    bool str = all_bounded_strings_valid(config, node);
    return str;
}

/*
bool betree_insert_all(struct betree* tree, size_t count, const char** exprs)
{
    // Hackish a bit for now, insert all except the last manually, then insert the last one legit
    struct sub** subs = calloc(count - 1, sizeof(*subs));
    for(size_t i = 0; i < count - 1; i++) {
        const char* expr = exprs[i];
        struct ast_node* node;
        if(parse(expr, &node) != 0) {
            fprintf(stderr, "Failed to parse id %" PRIu64 ": %s\n", i, expr);
            abort();
        }
        if(!is_valid(tree->config, node)) {
            return false;
        }
        assign_variable_id(tree->config, node);
        assign_str_id(tree->config, node);
        sort_lists(node);
        assign_pred_id(tree->config, node);
        struct sub* sub = make_sub(tree->config, i, node);
        subs[i] = sub;
    }
    tree->cnode->lnode->sub_count = count - 1;
    tree->cnode->lnode->subs = subs;
    return betree_insert(tree, count - 1, exprs[count - 1]);
}
*/
bool betree_insert_with_constants(struct betree* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        return false;
    }
    if(!is_valid(tree->config, node)) {
        free_ast_node(node);
        return false;
    }
    if(!assign_constants(constant_count, constants, node)) {
        return false;
    }
    assign_variable_id(tree->config, node);
    assign_str_id(tree->config, node);
    sort_lists(node);
    assign_pred_id(tree->config, node);
    struct sub* sub = make_sub(tree->config, id, node);
    return insert_be_tree(tree->config, sub, tree->cnode, NULL);
}

bool betree_insert(struct betree* tree, betree_sub_t id, const char* expr)
{
    return betree_insert_with_constants(tree, id, 0, NULL, expr);
}

static struct pred** make_environment(size_t attr_domain_count, const struct event* event)
{
    struct pred** preds = calloc(attr_domain_count, sizeof(*preds));
    for(size_t i = 0; i < event->pred_count; i++) {
        if(event->preds[i] != NULL) {
            preds[event->preds[i]->attr_var.var] = event->preds[i];
        }
    }
    return preds;
}

void betree_search_with_event(const struct betree* tree, const struct event* event, struct report* report)
{
    const struct pred** preds = (const struct pred**)make_environment(tree->config->attr_domain_count, event);
    betree_search_with_preds(tree->config, preds, tree->cnode, report);
}

void betree_search(const struct betree* tree, const char* event_str, struct report* report)
{
    struct event* event = make_event_from_string(tree->config, event_str);
    betree_search_with_event(tree, event, report);
    free_event(event);
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

void betree_add_integer_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max)
{
    add_attr_domain_bounded_i(betree->config, name, allow_undefined, min, max);
}

void betree_add_float_variable(struct betree* betree, const char* name, bool allow_undefined, double min, double max)
{
    add_attr_domain_bounded_f(betree->config, name, allow_undefined, min, max);
}

void betree_add_string_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count)
{
    add_attr_domain_bounded_s(betree->config, name, allow_undefined, count);
}

void betree_add_integer_list_variable(struct betree* betree, const char* name, bool allow_undefined, int64_t min, int64_t max)
{
    add_attr_domain_bounded_il(betree->config, name, allow_undefined, min, max);
}

void betree_add_string_list_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count)
{
    add_attr_domain_bounded_sl(betree->config, name, allow_undefined, count);
}

void betree_add_segments_variable(struct betree* betree, const char* name, bool allow_undefined)
{
    add_attr_domain_segments(betree->config, name, allow_undefined);
}

void betree_add_frequency_caps_variable(struct betree* betree, const char* name, bool allow_undefined)
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
    struct value value = { .value_type = VALUE_I, .ivalue = ivalue };
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

