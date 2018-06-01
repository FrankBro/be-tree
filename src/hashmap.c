#include <stdio.h>

#include "ast.h"
#include "hashmap.h"
#include "printer.h"
#include "utils.h"

void add_predicate_to_container(struct pred_container* container, struct ast_node* node)
{
    if(container->count == 0) {
        container->preds = calloc(1, sizeof(*container->preds));
        if(container->preds == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct ast_node** preds = realloc(
            container->preds, sizeof(*preds) * (container->count + 1));
        if(preds == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        container->preds = preds;
    }
    container->preds[container->count] = node;
    container->count++;
}

void add_predicate_to_map(struct pred_map* pred_map, struct ast_node* node)
{
    if(pred_map->pred_count == 0) {
        pred_map->preds = calloc(1, sizeof(*pred_map->preds));
        if(pred_map->preds == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct ast_node** preds = realloc(
            pred_map->preds, sizeof(*preds) * (pred_map->pred_count + 1));
        if(preds == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        pred_map->preds = preds;
    }
    pred_map->preds[pred_map->pred_count] = node;
    pred_map->pred_count++;
}

extern bool MATCH_NODE_DEBUG;

void add_predicate(struct pred_map* map, struct pred_container* container, struct ast_node* node)
{
    node->id = map->pred_count;
    struct ast_node* clone = clone_node(node);
    add_predicate_to_container(container, clone);
    add_predicate_to_map(map, clone);
}

void assign_numeric_compare_pred(struct pred_map* pred_map, struct ast_numeric_compare_expr* typed, struct ast_node* node)
{
    switch(typed->op) {
        case AST_NUMERIC_COMPARE_LT:
            for(size_t i = 0; i < pred_map->numeric_compare_map.lt_preds.count; i++) {
                if(eq_expr(node, pred_map->numeric_compare_map.lt_preds.preds[i])) {
                    node->id = pred_map->numeric_compare_map.lt_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->numeric_compare_map.lt_preds, node);
            break;
        case AST_NUMERIC_COMPARE_LE:
            for(size_t i = 0; i < pred_map->numeric_compare_map.le_preds.count; i++) {
                if(eq_expr(node, pred_map->numeric_compare_map.le_preds.preds[i])) {
                    node->id = pred_map->numeric_compare_map.le_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->numeric_compare_map.le_preds, node);
            break;
        case AST_NUMERIC_COMPARE_GT:
            for(size_t i = 0; i < pred_map->numeric_compare_map.gt_preds.count; i++) {
                if(eq_expr(node, pred_map->numeric_compare_map.gt_preds.preds[i])) {
                    node->id = pred_map->numeric_compare_map.gt_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->numeric_compare_map.gt_preds, node);
            break;
        case AST_NUMERIC_COMPARE_GE:
            for(size_t i = 0; i < pred_map->numeric_compare_map.ge_preds.count; i++) {
                if(eq_expr(node, pred_map->numeric_compare_map.ge_preds.preds[i])) {
                    node->id = pred_map->numeric_compare_map.ge_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->numeric_compare_map.ge_preds, node);
            break;
        default:
            switch_default_error("Invalid numeric compare op");
            break;
    }
}

void assign_equality_pred(struct pred_map* pred_map, struct ast_equality_expr* typed, struct ast_node* node)
{
    switch(typed->op) {
        case AST_EQUALITY_EQ:
            for(size_t i = 0; i < pred_map->equality_map.eq_preds.count; i++) {
                if(eq_expr(node, pred_map->equality_map.eq_preds.preds[i])) {
                    node->id = pred_map->equality_map.eq_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->equality_map.eq_preds, node);
            break;
        case AST_EQUALITY_NE:
            for(size_t i = 0; i < pred_map->equality_map.ne_preds.count; i++) {
                if(eq_expr(node, pred_map->equality_map.ne_preds.preds[i])) {
                    node->id = pred_map->equality_map.ne_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->equality_map.ne_preds, node);
            break;
        default:
            switch_default_error("Invalid equality op");
            break;
    }
}

void assign_bool_pred(struct pred_map* pred_map, struct ast_bool_expr* typed, struct ast_node* node)
{
    switch(typed->op) {
        case AST_BOOL_NOT:
            for(size_t i = 0; i < pred_map->bool_map.not_preds.count; i++) {
                if(eq_expr(node, pred_map->bool_map.not_preds.preds[i])) {
                    node->id = pred_map->bool_map.not_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->bool_map.not_preds, node);
            break;
        case AST_BOOL_OR:
            for(size_t i = 0; i < pred_map->bool_map.or_preds.count; i++) {
                if(eq_expr(node, pred_map->bool_map.or_preds.preds[i])) {
                    node->id = pred_map->bool_map.or_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->bool_map.or_preds, node);
            break;
        case AST_BOOL_AND:
            for(size_t i = 0; i < pred_map->bool_map.and_preds.count; i++) {
                if(eq_expr(node, pred_map->bool_map.and_preds.preds[i])) {
                    node->id = pred_map->bool_map.and_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->bool_map.and_preds, node);
            break;
        case AST_BOOL_VARIABLE:
            for(size_t i = 0; i < pred_map->bool_map.var_preds.count; i++) {
                if(eq_expr(node, pred_map->bool_map.var_preds.preds[i])) {
                    node->id = pred_map->bool_map.var_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->bool_map.var_preds, node);
            break;
        default:
            switch_default_error("Invalid bool op");
            break;
    }
}

void assign_set_pred(struct pred_map* pred_map, struct ast_set_expr* typed, struct ast_node* node)
{
    switch(typed->op) {
        case AST_SET_NOT_IN:
            for(size_t i = 0; i < pred_map->set_map.not_in_preds.count; i++) {
                if(eq_expr(node, pred_map->set_map.not_in_preds.preds[i])) {
                    node->id = pred_map->set_map.not_in_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->set_map.not_in_preds, node);
            break;
        case AST_SET_IN:
            for(size_t i = 0; i < pred_map->set_map.in_preds.count; i++) {
                if(eq_expr(node, pred_map->set_map.in_preds.preds[i])) {
                    node->id = pred_map->set_map.in_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->set_map.in_preds, node);
            break;
        default:
            switch_default_error("Invalid set op");
            break;
    }
}

void assign_list_pred(struct pred_map* pred_map, struct ast_list_expr* typed, struct ast_node* node)
{
    switch(typed->op) {
        case AST_LIST_ALL_OF:
            for(size_t i = 0; i < pred_map->list_map.all_of_preds.count; i++) {
                if(eq_expr(node, pred_map->list_map.all_of_preds.preds[i])) {
                    node->id = pred_map->list_map.all_of_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->list_map.all_of_preds, node);
            break;
        case AST_LIST_ONE_OF:
            for(size_t i = 0; i < pred_map->list_map.one_of_preds.count; i++) {
                if(eq_expr(node, pred_map->list_map.one_of_preds.preds[i])) {
                    node->id = pred_map->list_map.one_of_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->list_map.one_of_preds, node);
            break;
        case AST_LIST_NONE_OF:
            for(size_t i = 0; i < pred_map->list_map.none_of_preds.count; i++) {
                if(eq_expr(node, pred_map->list_map.none_of_preds.preds[i])) {
                    node->id = pred_map->list_map.none_of_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->list_map.none_of_preds, node);
            break;
        default:
            switch_default_error("Invalid list op");
            break;
    }
}

void assign_special_pred(struct pred_map* pred_map, struct ast_special_expr* typed, struct ast_node* node)
{
    switch(typed->type) {
        case AST_SPECIAL_STRING:
            for(size_t i = 0; i < pred_map->special_map.string_preds.count; i++) {
                if(eq_expr(node, pred_map->special_map.string_preds.preds[i])) {
                    node->id = pred_map->special_map.string_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->special_map.string_preds, node);
            break;
        case AST_SPECIAL_SEGMENT:
            for(size_t i = 0; i < pred_map->special_map.segment_preds.count; i++) {
                if(eq_expr(node, pred_map->special_map.segment_preds.preds[i])) {
                    node->id = pred_map->special_map.segment_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->special_map.segment_preds, node);
            break;
        case AST_SPECIAL_GEO:
            for(size_t i = 0; i < pred_map->special_map.geo_preds.count; i++) {
                if(eq_expr(node, pred_map->special_map.geo_preds.preds[i])) {
                    node->id = pred_map->special_map.geo_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->special_map.geo_preds, node);
            break;
        case AST_SPECIAL_FREQUENCY:
            for(size_t i = 0; i < pred_map->special_map.frequency_preds.count; i++) {
                if(eq_expr(node, pred_map->special_map.frequency_preds.preds[i])) {
                    node->id = pred_map->special_map.frequency_preds.preds[i]->id;
                    return;
                }
            }
            add_predicate(pred_map, &pred_map->special_map.frequency_preds, node);
            break;
        default:
            switch_default_error("Invalid list op");
            break;
    }
}

void assign_pred(struct pred_map* pred_map, struct ast_node* node)
{
    if(node->type == AST_TYPE_BOOL_EXPR) {
        if(node->bool_expr.op == AST_BOOL_NOT) {
            assign_pred(pred_map, node->bool_expr.unary.expr);
        }
        else if(node->bool_expr.op == AST_BOOL_OR || node->bool_expr.op == AST_BOOL_AND) {
            assign_pred(pred_map, node->bool_expr.binary.lhs);
            assign_pred(pred_map, node->bool_expr.binary.rhs);
        }
    }
    switch(node->type) {
        case AST_TYPE_NUMERIC_COMPARE_EXPR:
            assign_numeric_compare_pred(pred_map, &node->numeric_compare_expr, node);
            break;
        case AST_TYPE_EQUALITY_EXPR:
            assign_equality_pred(pred_map, &node->equality_expr, node);
            break;
        case AST_TYPE_BOOL_EXPR:
            assign_bool_pred(pred_map, &node->bool_expr, node);
            break;
        case AST_TYPE_SET_EXPR:
            assign_set_pred(pred_map, &node->set_expr, node);
            break;
        case AST_TYPE_LIST_EXPR:
            assign_list_pred(pred_map, &node->list_expr, node);
            break;
        case AST_TYPE_SPECIAL_EXPR:
            assign_special_pred(pred_map, &node->special_expr, node);
            break;
        default:
            switch_default_error("Invalid node type");
            break;
    }
}

void init_pred_container(struct pred_container* container)
{
    container->count = 0;
    container->preds = NULL;
}

struct pred_map* make_pred_map()
{
    struct pred_map* pred_map = calloc(1, sizeof(*pred_map));
    if(pred_map == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    pred_map->pred_count = 0;
    pred_map->preds = NULL;
    init_pred_container(&pred_map->numeric_compare_map.ge_preds);
    init_pred_container(&pred_map->numeric_compare_map.gt_preds);
    init_pred_container(&pred_map->numeric_compare_map.le_preds);
    init_pred_container(&pred_map->numeric_compare_map.lt_preds);
    init_pred_container(&pred_map->equality_map.eq_preds);
    init_pred_container(&pred_map->equality_map.ne_preds);
    init_pred_container(&pred_map->set_map.in_preds);
    init_pred_container(&pred_map->set_map.not_in_preds);
    init_pred_container(&pred_map->list_map.all_of_preds);
    init_pred_container(&pred_map->list_map.none_of_preds);
    init_pred_container(&pred_map->list_map.one_of_preds);
    init_pred_container(&pred_map->bool_map.or_preds);
    init_pred_container(&pred_map->bool_map.not_preds);
    init_pred_container(&pred_map->bool_map.and_preds);
    init_pred_container(&pred_map->bool_map.var_preds);
    init_pred_container(&pred_map->special_map.frequency_preds);
    init_pred_container(&pred_map->special_map.geo_preds);
    init_pred_container(&pred_map->special_map.segment_preds);
    init_pred_container(&pred_map->special_map.string_preds);
    return pred_map;
}

void free_pred_map(struct pred_map* pred_map)
{
    free(pred_map->numeric_compare_map.ge_preds.preds);
    free(pred_map->numeric_compare_map.gt_preds.preds);
    free(pred_map->numeric_compare_map.le_preds.preds);
    free(pred_map->numeric_compare_map.lt_preds.preds);
    free(pred_map->equality_map.eq_preds.preds);
    free(pred_map->equality_map.ne_preds.preds);
    free(pred_map->set_map.in_preds.preds);
    free(pred_map->set_map.not_in_preds.preds);
    free(pred_map->list_map.all_of_preds.preds);
    free(pred_map->list_map.one_of_preds.preds);
    free(pred_map->list_map.none_of_preds.preds);
    free(pred_map->bool_map.and_preds.preds);
    free(pred_map->bool_map.not_preds.preds);
    free(pred_map->bool_map.or_preds.preds);
    free(pred_map->bool_map.var_preds.preds);
    free(pred_map->special_map.frequency_preds.preds);
    free(pred_map->special_map.geo_preds.preds);
    free(pred_map->special_map.segment_preds.preds);
    free(pred_map->special_map.string_preds.preds);
    for(size_t i = 0; i < pred_map->pred_count; i++) {
        free_ast_node(pred_map->preds[i]);
    }
    free(pred_map->preds);
    free(pred_map);
}

