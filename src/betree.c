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

bool betree_delete(betree_sub_t id, struct betree* betree)
{
    struct sub* sub = find_sub_id(id, betree->cnode);
    betree_assert(betree->config->abort_on_error, ERROR_CANT_FIND_SUB, sub != NULL);
    bool found = betree_delete_inner(betree->config, sub, betree->cnode);
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

bool betree_insert(betree_sub_t id, const char* expr, struct betree* tree)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        fprintf(stderr, "Failed to parse id %" PRIu64 ": %s\n", id, expr);
        if(tree->config->abort_on_error) {
            abort();
        }
    }
    if(!is_valid(tree->config, node)) {
        return false;
    }
    assign_variable_id(tree->config, node);
    assign_str_id(tree->config, node);
    assign_pred_id(tree->config, node);
    struct sub* sub = make_sub(tree->config, id, node);
    return insert_be_tree(tree->config, sub, tree->cnode, NULL);
}

void betree_search(const struct betree* tree, const char* event_str, struct report* report)
{
    struct event* event = make_event_from_string(tree->config, event_str);
    betree_search_with_event(tree->config, event, tree->cnode, report);
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

