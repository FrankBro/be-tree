#include <ctype.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "betree.h"
#include "error.h"
#include "hashmap.h"
#include "tree.h"
#include "utils.h"
#include "value.h"

/*bool betree_delete(struct betree* betree, betree_sub_t id)*/
/*{*/
    /*struct betree_sub* sub = find_sub_id(id, betree->cnode);*/
    /*bool found = betree_delete_inner(betree->config->attr_domain_count, (const struct attr_domain**)betree->config->attr_domains, sub, betree->cnode);*/
    /*free_sub(sub);*/
    /*return found;*/
/*}*/

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct betree_event** event);

static bool is_valid(const struct config* config, const struct ast_node* node)
{
    bool var = all_variables_in_config(config, node);
    if(!var) {
        fprintf(stderr, "Missing variable in config\n");
        return false;
    }
    bool str = all_bounded_strings_valid(config, node);
    if(!str) {
        fprintf(stderr, "Out of bound string\n");
        return false;
    }
    return true;
}

/*
bool betree_insert_all(struct betree* tree, size_t count, const char** exprs)
{
    // Hackish a bit for now, insert all except the last manually, then insert the last one legit
    struct sub** subs = bcalloc(count - 1, sizeof(*subs));
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

static void fix_float_with_no_fractions(struct config* config, struct ast_node* node)
{
    switch(node->type) {
        case AST_TYPE_COMPARE_EXPR: {
            bool is_value_int = 
                node->compare_expr.value.value_type == AST_COMPARE_VALUE_INTEGER;
            bool is_domain_float = 
                config->attr_domains[node->compare_expr.attr_var.var]->bound.value_type == BETREE_FLOAT;
            if(is_value_int && is_domain_float) {
                node->compare_expr.value.value_type = AST_COMPARE_VALUE_FLOAT;
                node->compare_expr.value.float_value = node->compare_expr.value.integer_value;
            }
            return;
        }
        case AST_TYPE_EQUALITY_EXPR: {
            bool is_value_int = 
                node->equality_expr.value.value_type == AST_EQUALITY_VALUE_INTEGER;
            bool is_domain_float = 
                config->attr_domains[node->equality_expr.attr_var.var]->bound.value_type == BETREE_FLOAT;
            if(is_value_int && is_domain_float) {
                node->equality_expr.value.value_type = AST_EQUALITY_VALUE_FLOAT;
                node->equality_expr.value.float_value = node->equality_expr.value.integer_value;
            }
            return;
        }
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR:
                case AST_BOOL_AND:
                    fix_float_with_no_fractions(config, node->bool_expr.binary.lhs);
                    fix_float_with_no_fractions(config, node->bool_expr.binary.rhs);
                    return;
                case AST_BOOL_NOT:
                    fix_float_with_no_fractions(config, node->bool_expr.unary.expr);
                    return;
                case AST_BOOL_LITERAL:
                case AST_BOOL_VARIABLE:
                default:
                    return;
            }
        case AST_TYPE_IS_NULL_EXPR:
        case AST_TYPE_LIST_EXPR:
        case AST_TYPE_SET_EXPR:
        case AST_TYPE_SPECIAL_EXPR:
        default:
            return;
    }
}

static struct value_bound boolean_bound(bool value)
{
    struct value_bound bound;
    bound.value_type = BETREE_BOOLEAN;
    bound.bmin = value;
    bound.bmax = value;
    return bound;
}

static struct value_bound integer_bound(int64_t value) 
{
    struct value_bound bound;
    bound.value_type = BETREE_INTEGER;
    bound.imin = value;
    bound.imax = value;
    return bound;
}

static struct value_bound integer_list_bound(int64_t min, int64_t max) 
{
    struct value_bound bound;
    bound.value_type = BETREE_INTEGER_LIST;
    bound.imin = min;
    bound.imax = max;
    return bound;
}

static struct value_bound float_bound(double value) 
{
    struct value_bound bound;
    bound.value_type = BETREE_FLOAT;
    bound.fmin = value;
    bound.fmax = value;
    return bound;
}

static struct value_bound string_bound(size_t value) 
{
    struct value_bound bound;
    bound.value_type = BETREE_STRING;
    bound.smin = value;
    bound.smax = value;
    return bound;
}

static struct value_bound string_list_bound(size_t min, size_t max) 
{
    struct value_bound bound;
    bound.value_type = BETREE_STRING_LIST;
    bound.smin = min;
    bound.smax = max;
    return bound;
}

static struct value_bound integer_enum_bound(size_t value) 
{
    struct value_bound bound;
    bound.value_type = BETREE_INTEGER_ENUM;
    bound.smin = value;
    bound.smax = value;
    return bound;
}

static struct value_bound compare_simple_bound(struct compare_value value, enum ast_compare_e op, bool inverted)
{
    switch(value.value_type) {
        case AST_COMPARE_VALUE_INTEGER: {
            int64_t i = value.integer_value;
            switch(op) {
                case AST_COMPARE_LT: return inverted ? compare_simple_bound(value, AST_COMPARE_GE, false) : integer_bound(i - 1);
                case AST_COMPARE_LE: return inverted ? compare_simple_bound(value, AST_COMPARE_GT, false) : integer_bound(i);
                case AST_COMPARE_GT: return inverted ? compare_simple_bound(value, AST_COMPARE_LE, false) : integer_bound(i + 1);
                case AST_COMPARE_GE: return inverted ? compare_simple_bound(value, AST_COMPARE_LT, false) : integer_bound(i);
                default: abort();
            }
        }
        case AST_COMPARE_VALUE_FLOAT: {
            double f = value.float_value;
            switch(op) {
                case AST_COMPARE_LT: return inverted ? compare_simple_bound(value, AST_COMPARE_GE, false) : float_bound(f - __DBL_EPSILON__);
                case AST_COMPARE_LE: return inverted ? compare_simple_bound(value, AST_COMPARE_GT, false) : float_bound(f);
                case AST_COMPARE_GT: return inverted ? compare_simple_bound(value, AST_COMPARE_LE, false) : float_bound(f + __DBL_EPSILON__);
                case AST_COMPARE_GE: return inverted ? compare_simple_bound(value, AST_COMPARE_LT, false) : float_bound(f);
                default: abort();
            }
        }
        default: abort();
    }
}

static struct value_bound equality_simple_bound(struct equality_value value)
{
    switch(value.value_type) {
        case AST_EQUALITY_VALUE_INTEGER: return integer_bound(value.integer_value);
        case AST_EQUALITY_VALUE_FLOAT:   return float_bound(value.float_value);
        case AST_EQUALITY_VALUE_STRING:  return string_bound(value.string_value.str);
        case AST_EQUALITY_VALUE_INTEGER_ENUM: return integer_enum_bound(value.integer_enum_value.ienum);
        default: abort();
    }
}

static struct value_bound set_simple_bound(struct set_left_value left_value, struct set_right_value right_value)
{
    if(left_value.value_type == AST_SET_LEFT_VALUE_VARIABLE) {
        switch(right_value.value_type) {
            case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
                int64_t imin = right_value.integer_list_value->integers[0];
                int64_t imax = right_value.integer_list_value->integers[right_value.integer_list_value->count - 1];
                return integer_list_bound(imin, imax);
            }
            case AST_SET_RIGHT_VALUE_STRING_LIST: {
                size_t smin = right_value.string_list_value->strings[0].str;
                size_t smax = right_value.string_list_value->strings[right_value.string_list_value->count - 1].str;
                return string_list_bound(smin, smax);
            }
            case AST_SET_RIGHT_VALUE_VARIABLE:
            default: abort ();
        }
    }
    else {
        switch(left_value.value_type) {
            case AST_SET_LEFT_VALUE_INTEGER: 
                return integer_bound(left_value.integer_value);
            case AST_SET_LEFT_VALUE_STRING:  
                return string_bound(left_value.string_value.str);
            case AST_SET_LEFT_VALUE_VARIABLE:
            default: abort();
        }
    }
}

static struct value_bound list_simple_bound(struct list_value value)
{
    switch(value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            int64_t imin = value.integer_list_value->integers[0];
            int64_t imax = value.integer_list_value->integers[value.integer_list_value->count - 1];
            return integer_list_bound(imin, imax);
        }
        case AST_LIST_VALUE_STRING_LIST: {
            size_t smin = value.string_list_value->strings[0].str;
            size_t smax = value.string_list_value->strings[value.string_list_value->count - 1].str;
            return string_list_bound(smin, smax);
        }
        default: abort();
    }
}

static bool might_be_affected(const struct ast_node* node, betree_var_t var)
{
    switch(node->type) {
        case AST_TYPE_COMPARE_EXPR: return node->compare_expr.attr_var.var == var;
        case AST_TYPE_EQUALITY_EXPR: return node->equality_expr.attr_var.var == var;
        case AST_TYPE_BOOL_EXPR:
            return
                node->bool_expr.op == AST_BOOL_OR || node->bool_expr.op == AST_BOOL_AND || node->bool_expr.op == AST_BOOL_NOT
                 || (node->bool_expr.op == AST_BOOL_VARIABLE && node->bool_expr.variable.var == var);
        case AST_TYPE_SET_EXPR: 
            return 
                (node->set_expr.left_value.value_type  == AST_SET_LEFT_VALUE_VARIABLE  && node->set_expr.left_value.variable_value.var  == var) ||
                (node->set_expr.right_value.value_type == AST_SET_RIGHT_VALUE_VARIABLE && node->set_expr.right_value.variable_value.var == var);
        case AST_TYPE_LIST_EXPR: return node->list_expr.attr_var.var == var;
        case AST_TYPE_SPECIAL_EXPR: return false;
        case AST_TYPE_IS_NULL_EXPR: return false;
        default: abort();
    }
}

static bool get_simple_variable_bound(betree_var_t var, const struct ast_node* node, bool inverted, struct value_bound* bound)
{
    if(!might_be_affected(node, var)) {
        return false;
    }
    switch(node->type) {
        case AST_TYPE_COMPARE_EXPR:
            *bound = compare_simple_bound(node->compare_expr.value, node->compare_expr.op, inverted);
            return true;
        case AST_TYPE_EQUALITY_EXPR:
            *bound = equality_simple_bound(node->equality_expr.value);
            return true;
        case AST_TYPE_BOOL_EXPR:
            switch(node->bool_expr.op) {
                case AST_BOOL_OR: 
                case AST_BOOL_AND: {
                    struct value_bound lbound, rbound;
                    bool ltouched = get_simple_variable_bound(var, node->bool_expr.binary.lhs, inverted, &lbound);
                    bool rtouched = get_simple_variable_bound(var, node->bool_expr.binary.rhs, inverted, &rbound);
                    if((ltouched || rtouched) == false) {
                        return false;
                    }
                    if(ltouched == false) {
                        *bound = rbound;
                        return true;
                    }
                    if(rtouched == false) {
                        *bound = lbound;
                        return true;
                    }
                    bound->value_type = lbound.value_type;
                    switch(bound->value_type) {
                        case BETREE_BOOLEAN:
                            bound->bmin = lbound.bmin < rbound.bmin ? lbound.bmin : rbound.bmin;
                            bound->bmax = lbound.bmax > rbound.bmax ? lbound.bmax : rbound.bmax;
                            break;
                        case BETREE_INTEGER:
                        case BETREE_INTEGER_LIST:
                            bound->imin = lbound.imin < rbound.imin ? lbound.imin : rbound.imin;
                            bound->imax = lbound.imax > rbound.imax ? lbound.imax : rbound.imax;
                            break;
                        case BETREE_FLOAT:
                            bound->fmin = lbound.fmin < rbound.fmin ? lbound.fmin : rbound.fmin;
                            bound->fmax = lbound.fmax > rbound.fmax ? lbound.fmax : rbound.fmax;
                            break;
                        case BETREE_STRING:
                        case BETREE_STRING_LIST:
                        case BETREE_INTEGER_ENUM:
                            bound->smin = lbound.smin < rbound.smin ? lbound.smin : rbound.smin;
                            bound->smax = lbound.smax > rbound.smax ? lbound.smax : rbound.smax;
                            break;
                        case BETREE_SEGMENTS:
                        case BETREE_FREQUENCY_CAPS:
                        default: abort();
                    }
                    return true;
                }
                case AST_BOOL_NOT:
                    return get_simple_variable_bound(var, node->bool_expr.unary.expr, !inverted, bound);
                case AST_BOOL_VARIABLE:
                    *bound = boolean_bound(!inverted);
                    return true;
                case AST_BOOL_LITERAL:
                    return false;
                default:
                    abort();
            }
            break;
        case AST_TYPE_SET_EXPR:
            *bound = set_simple_bound(node->set_expr.left_value, node->set_expr.right_value);
            return true;
        case AST_TYPE_LIST_EXPR:
            *bound = list_simple_bound(node->list_expr.value);
            return true;
        case AST_TYPE_SPECIAL_EXPR:
            return false;
        case AST_TYPE_IS_NULL_EXPR:
            return false;
        default: 
            abort();
    }
    return false;
}

static void change_boundaries(struct config* config, const struct ast_node* node) 
{
    // Use function to extract boundaries, THEN apply them to the config
    for(size_t i = 0; i < config->attr_domain_count; i++) {
        struct attr_domain* attr_domain = config->attr_domains[i];
        if(attr_domain->bound.value_type == BETREE_BOOLEAN || attr_domain->bound.value_type == BETREE_SEGMENTS || attr_domain->bound.value_type == BETREE_FREQUENCY_CAPS) {
            continue;
        }
        struct value_bound bound;
        bool will_affect = get_simple_variable_bound(attr_domain->attr_var.var, node, false, &bound);
        if(!will_affect) {
            continue;
        }
        switch(attr_domain->bound.value_type) {
            case BETREE_BOOLEAN:
                attr_domain->bound.bmin = bound.bmin < attr_domain->bound.bmin ? bound.bmin : attr_domain->bound.bmin;
                attr_domain->bound.bmax = bound.bmax < attr_domain->bound.bmax ? bound.bmax : attr_domain->bound.bmax;
                break;
            case BETREE_INTEGER:
            case BETREE_INTEGER_LIST:
                if(attr_domain->bound.imin != INT64_MIN) {
                    attr_domain->bound.imin = bound.imin < attr_domain->bound.imin ? bound.imin : attr_domain->bound.imin;
                }
                else {
                    attr_domain->bound.imin = bound.imin;
                }
                if(attr_domain->bound.imax != INT64_MAX) {
                    attr_domain->bound.imax = bound.imax > attr_domain->bound.imax ? bound.imax : attr_domain->bound.imax;
                }
                else {
                    attr_domain->bound.imax = bound.imax;
                }
                break;
            case BETREE_FLOAT:
                if(fne(attr_domain->bound.fmin, -DBL_MAX)) {
                    attr_domain->bound.fmin = bound.fmin < attr_domain->bound.fmin ? bound.fmin : attr_domain->bound.fmin;
                }
                else {
                    attr_domain->bound.fmin = bound.fmin;
                }
                if(fne(attr_domain->bound.fmax, DBL_MAX)) {
                    attr_domain->bound.fmax = bound.fmax > attr_domain->bound.fmax ? bound.fmax : attr_domain->bound.fmax;
                }
                else {
                    attr_domain->bound.fmax = bound.fmax;
                }
                break;
            case BETREE_STRING:
            case BETREE_STRING_LIST:
                for(size_t j = 0; j < config->string_map_count; j++) {
                    if(config->string_maps[j].attr_var.var == attr_domain->attr_var.var) {
                        size_t smax = config->string_maps[j].string_value_count - 1;
                        if(attr_domain->bound.smax < SIZE_MAX - 1) {
                            attr_domain->bound.smax = smax > attr_domain->bound.smax ? smax : attr_domain->bound.smax;
                        }
                        else {
                            attr_domain->bound.smax = smax;
                        }
                    }
                }
                break;
            case BETREE_INTEGER_ENUM:
                for(size_t j = 0; j < config->integer_map_count; j++) {
                    if(config->integer_maps[j].attr_var.var == attr_domain->attr_var.var) {
                        size_t smax = config->integer_maps[j].integer_value_count - 1;
                        if(attr_domain->bound.smax < SIZE_MAX - 1) {
                            attr_domain->bound.smax = smax > attr_domain->bound.smax ? smax : attr_domain->bound.smax;
                        }
                        else {
                            attr_domain->bound.smax = smax;
                        }
                    }
                }
                break;
            case BETREE_SEGMENTS:
                break;
            case BETREE_FREQUENCY_CAPS:
                break;
            default:
                break;
        }
    }
}

bool betree_change_boundaries(struct betree* tree, const char* expr)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        return false;
    }
    assign_variable_id(tree->config, node);
    assign_str_id(tree->config, node, true);
    assign_ienum_id(tree->config, node, true);
    sort_lists(node);
    fix_float_with_no_fractions(tree->config, node);
    change_boundaries(tree->config, node);
    free_ast_node(node);
    return true;
}

bool betree_insert_with_constants(struct betree* tree,
    betree_sub_t id,
    size_t constant_count,
    const struct betree_constant** constants,
    const char* expr)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        fprintf(stderr, "Can't parse %lu\n", id);
        return false;
    }
    assign_variable_id(tree->config, node);
    if(!is_valid(tree->config, node)) {
        fprintf(stderr, "Can't validate %lu\n", id);
        free_ast_node(node);
        return false;
    }
    if(!assign_constants(constant_count, constants, node)) {
        fprintf(stderr, "Can't assign constants %lu\n", id);
        return false;
    }
    assign_str_id(tree->config, node, false);
    assign_ienum_id(tree->config, node, false);
    sort_lists(node);
    fix_float_with_no_fractions(tree->config, node);
    assign_pred_id(tree->config, node);
    struct betree_sub* sub = make_sub(tree->config, id, node);
    return insert_be_tree(tree->config, sub, tree->cnode, NULL);
}

const struct betree_sub* betree_make_sub(struct betree* tree, betree_sub_t id, size_t constant_count, const struct betree_constant** constants, const char* expr)
{
    struct ast_node* node;
    if(parse(expr, &node) != 0) {
        fprintf(stderr, "Can't parse %lu\n", id);
        return NULL;
    }
    assign_variable_id(tree->config, node);
    if(!all_variables_in_config(tree->config, node)) {
        fprintf(stderr, "Missing variable in config\n");
        free_ast_node(node);
        return NULL;
    }
    assign_str_id(tree->config, node, true);
    assign_ienum_id(tree->config, node, true);
    fix_float_with_no_fractions(tree->config, node);
    if(!all_exprs_valid(tree->config, node)) {
        fprintf(stderr, "Invalid expression found\n");
        free_ast_node(node);
        return NULL;
    }
    if(!assign_constants(constant_count, constants, node)) {
        fprintf(stderr, "Can't assign constants %lu\n", id);
        free_ast_node(node);
        return NULL;
    }
    sort_lists(node);
    change_boundaries(tree->config, node);
    assign_pred_id(tree->config, node);
    struct betree_sub* sub = make_sub(tree->config, id, node);
    return sub;
}

bool betree_insert_sub(struct betree* tree, const struct betree_sub* sub)
{
    return insert_be_tree(tree->config, sub, tree->cnode, NULL);
}

bool betree_insert(struct betree* tree, betree_sub_t id, const char* expr)
{
    return betree_insert_with_constants(tree, id, 0, NULL, expr);
}

const struct betree_variable** make_environment(size_t attr_domain_count, const struct betree_event* event)
{
    const struct betree_variable** preds = bcalloc(attr_domain_count * sizeof(*preds));
    for(size_t i = 0; i < event->variable_count; i++) {
        if(event->variables[i] != NULL) {
            preds[event->variables[i]->attr_var.var] = event->variables[i];
        }
    }
    return preds;
}

static bool betree_search_with_event_filled(const struct betree* betree, struct betree_event* event, struct report* report)
{
    const struct betree_variable** variables
        = make_environment(betree->config->attr_domain_count, event);
    if(validate_variables(betree->config, variables) == false) {
        fprintf(stderr, "Failed to validate event\n");
        return false;
    }
    return betree_search_with_preds(betree->config, variables, betree->cnode, report);
}

bool betree_search_with_event_filled_ids(const struct betree* betree, struct betree_event* event, struct report* report, const uint64_t* ids, size_t sz)
{
    const struct betree_variable** variables
        = make_environment(betree->config->attr_domain_count, event);
    if(validate_variables(betree->config, variables) == false) {
        fprintf(stderr, "Failed to validate event\n");
        return false;
    }
    return betree_search_with_preds_ids(betree->config, variables, betree->cnode, report, ids, sz);
}


static bool betree_exists_with_event_filled(const struct betree* betree, struct betree_event* event)
{
    const struct betree_variable** variables = make_environment(betree->config->attr_domain_count, event);
    return betree_exists_with_preds(betree->config, variables, betree->cnode);
}

bool betree_exists(const struct betree* tree, const char* event_str)
{
    struct betree_event* event = make_event_from_string(tree, event_str);
    bool result = betree_exists_with_event_filled(tree, event);
    free_event(event);
    return result;
}

bool betree_exists_with_event(const struct betree* betree, struct betree_event* event)
{
    fill_event(betree->config, event);
    sort_event_lists(event);
    return betree_exists_with_event_filled(betree, event);
}

bool betree_search(const struct betree* tree, const char* event_str, struct report* report)
{
    struct betree_event* event = make_event_from_string(tree, event_str);
    bool result = betree_search_with_event_filled(tree, event, report);
    free_event(event);
    return result;
}

bool betree_search_ids(const struct betree* tree, const char* event_str, struct report* report, const uint64_t* ids, size_t sz)
{
    struct betree_event* event = make_event_from_string(tree, event_str);
    bool result = betree_search_with_event_filled_ids(tree, event, report, ids, sz);
    free_event(event);
    return result;
}

bool betree_search_with_event(const struct betree* betree, struct betree_event* event, struct report* report)
{
    fill_event(betree->config, event);
    sort_event_lists(event);
    return betree_search_with_event_filled(betree, event, report);
}

bool betree_search_with_event_ids(const struct betree* betree, struct betree_event* event, struct report* report, const uint64_t* ids, size_t sz)
{
    fill_event(betree->config, event);
    sort_event_lists(event);
    return betree_search_with_event_filled_ids(betree, event, report, ids, sz);
}

struct report* make_report()
{
    struct report* report = bcalloc(sizeof(*report));
    if(report == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
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
    bfree(report->subs);
    bfree(report);
}

struct report_counting* make_report_counting()
{
    struct report_counting* report = bcalloc(sizeof(*report));
    if(report == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    report->evaluated = 0;
    report->matched = 0;
    report->memoized = 0;
    report->shorted = 0;
    report->subs = NULL;
    report->node_count = 0;
    report->ops_count = 0;
    return report;
}

void free_report_counting(struct report_counting* report)
{
    bfree(report->subs);
    bfree(report);
}

static void betree_init_with_config(struct betree* betree, struct config* config)
{
    betree->config = config;
    betree->cnode = make_cnode(betree->config, NULL);
}

void betree_init(struct betree* betree)
{
    struct config* config = make_default_config();
    betree_init_with_config(betree, config);
}

static struct betree* betree_make_with_config(struct config* config)
{
    struct betree* tree = bcalloc(sizeof(*tree));
    if(tree == NULL) {
        fprintf(stderr, "%s bcalloc failed\n", __func__);
        abort();
    }
    betree_init_with_config(tree, config);
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

void betree_deinit(struct betree* betree)
{
    free_cnode(betree->cnode);
    free_config(betree->config);
}

void betree_free(struct betree* betree)
{
    betree_deinit(betree);
    bfree(betree);
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

void betree_add_integer_enum_variable(struct betree* betree, const char* name, bool allow_undefined, size_t count)
{
    add_attr_domain_bounded_ie(betree->config, name, allow_undefined, count);
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

struct betree_constant* betree_make_integer_constant(const char* name, int64_t integer_value)
{
    struct betree_constant* constant = bmalloc(sizeof(*constant));
    if(constant == NULL) {
        fprintf(stderr, "%s bmalloc failed", __func__);
        abort();
    }
    constant->name = bstrdup(name);
    struct value value = { .value_type = BETREE_INTEGER, .integer_value = integer_value };
    constant->value = value;
    return constant;
}

void betree_free_constant(struct betree_constant* constant)
{
    free_value(constant->value);
    bfree((char*)constant->name);
    bfree(constant);
}

void betree_free_constants(size_t count, struct betree_constant** constants)
{
    for(size_t i = 0; i < count; i++) {
        if(constants[i] != NULL) {
            betree_free_constant(constants[i]);
        }
    }
}

struct betree_integer_list* betree_make_integer_list(size_t count)
{
    struct betree_integer_list* list = bmalloc(sizeof(*list));
    list->count = count;
    list->integers = bcalloc(count * sizeof(*list->integers));
    return list;
}

void betree_add_integer(struct betree_integer_list* list, size_t index, int64_t value)
{
    list->integers[index] = value;
}

struct betree_string_list* betree_make_string_list(size_t count)
{
    struct betree_string_list* list = bmalloc(sizeof(*list));
    list->count = count;
    list->strings = bcalloc(count * sizeof(*list->strings));
    return list;
}

void betree_add_string(struct betree_string_list* list, size_t index, const char* value)
{
    struct string_value s = { .string = bstrdup(value) };
    list->strings[index] = s;
}

struct betree_segments* betree_make_segments(size_t count)
{
    struct betree_segments* segments = bmalloc(sizeof(*segments));
    segments->size = count;
    segments->content = bcalloc(count * sizeof(*segments->content));
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
    struct betree_frequency_caps* frequency_caps = bmalloc(sizeof(*frequency_caps));
    frequency_caps->size = count;
    frequency_caps->content = bcalloc(count * sizeof(*frequency_caps->content));
    return frequency_caps;
}

struct betree_frequency_cap* betree_make_frequency_cap(const char* stype,
    uint32_t id,
    const char* ns,
    bool timestamp_defined,
    int64_t timestamp,
    uint32_t value)
{
    enum frequency_type_e type = get_type_from_string(stype);
    if(type == FREQUENCY_TYPE_INVALID) {
        return NULL;
    }
    struct string_value namespace
        = { .string = bstrdup(ns), .str = INVALID_STR, .var = INVALID_VAR };
    return make_frequency_cap_with_type(type, id, namespace, timestamp_defined, timestamp, value);
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
    struct betree_variable* var = bmalloc(sizeof(*var));
    var->attr_var = attr_var;
    var->value = value;
    return var;
}

struct betree_variable* betree_make_boolean_variable(const char* name, bool value)
{
    struct value v = { .value_type = BETREE_BOOLEAN, .boolean_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_integer_variable(const char* name, int64_t value)
{
    struct value v = { .value_type = BETREE_INTEGER, .integer_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_float_variable(const char* name, double value)
{
    struct value v = { .value_type = BETREE_FLOAT, .float_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_string_variable(const char* name, const char* value)
{
    struct string_value string_value = { .string = bstrdup(value), .var = INVALID_VAR, .str = INVALID_STR };
    struct value v = { .value_type = BETREE_STRING, .string_value = string_value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_integer_list_variable(
    const char* name, struct betree_integer_list* value)
{
    struct value v = { .value_type = BETREE_INTEGER_LIST, .integer_list_value = value };
    return betree_make_variable(name, v);
}

struct betree_variable* betree_make_string_list_variable(
    const char* name, struct betree_string_list* value)
{
    struct value v = { .value_type = BETREE_STRING_LIST, .string_list_value = value };
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
    struct value v = { .value_type = BETREE_FREQUENCY_CAPS, .frequency_caps_value = value };
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
    bfree((char*)variable->attr_var.attr);
    free_value(variable->value);
    bfree(variable);
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
    struct betree_event* event = bmalloc(sizeof(*event));
    event->variable_count = betree->config->attr_domain_count;
    event->variables = bcalloc(event->variable_count * sizeof(*event->variables));
    return event;
}

void betree_set_variable(struct betree_event* event, size_t index, struct betree_variable* variable)
{
    event->variables[index] = variable;
}

