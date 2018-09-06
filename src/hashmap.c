#include <stdio.h>

#include "ast.h"
#include "clone.h"
#include "hashmap.h"
#include "printer.h"
#include "utils.h"

static void add_predicate_to_container(struct pred_container* container, struct ast_node* node)
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

static void add_predicate_to_map(struct pred_map* pred_map, struct ast_node* node)
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

static void match_or_insert(struct pred_map* pred_map, struct pred_container* container, struct ast_node* node)
{
    for(size_t i = 0; i < container->count; i++) {
        struct ast_node* other_node = container->preds[i];
        if(eq_expr(node, other_node)) {
            if(other_node->id == INVALID_PRED) {
                other_node->id = pred_map->pred_count;
                add_predicate_to_map(pred_map, other_node);
            }
            node->id = other_node->id;
            return;
        }
    }
    add_predicate_to_container(container, node);
}

static void assign_compare_pred(struct pred_map* pred_map, struct ast_compare_expr* typed, struct ast_node* node)
{
    struct pred_compare_map* m = &pred_map->compare_map;
    switch(typed->op) {
        case AST_COMPARE_LT: 
            match_or_insert(pred_map, &m->lt_preds, node); 
            break;
        case AST_COMPARE_LE: 
            match_or_insert(pred_map, &m->le_preds, node); 
            break;
        case AST_COMPARE_GT: 
            match_or_insert(pred_map, &m->gt_preds, node); 
            break;
        case AST_COMPARE_GE: 
            match_or_insert(pred_map, &m->ge_preds, node); 
            break;
        default:
            switch_default_error("Invalid compare op");
            break;
    }
}

static void assign_equality_pred(struct pred_map* pred_map, struct ast_equality_expr* typed, struct ast_node* node)
{
    struct pred_equality_map* m = &pred_map->equality_map;
    switch(typed->op) {
        case AST_EQUALITY_EQ: 
            match_or_insert(pred_map, &m->eq_preds, node); 
            break;
        case AST_EQUALITY_NE: 
            match_or_insert(pred_map, &m->ne_preds, node); 
            break;
        default:
            switch_default_error("Invalid equality op");
            break;
    }
}

// BIG assumption, the inner expressions have been assigned a pred. So we save a bunch of time for "and", "or" and "not"
// not anymore but should redo it to improve insert time
static void assign_bool_pred(struct pred_map* pred_map, struct ast_bool_expr* typed, struct ast_node* node)
{
    struct pred_bool_map* m = &pred_map->bool_map;
    switch(typed->op) {
        case AST_BOOL_LITERAL:
            match_or_insert(pred_map, &m->literal_preds, node); 
            break;
        case AST_BOOL_NOT: 
            match_or_insert(pred_map, &m->not_preds, node); 
            break;
        case AST_BOOL_OR:  
            match_or_insert(pred_map, &m->or_preds, node); 
            break;
        case AST_BOOL_AND: 
            match_or_insert(pred_map, &m->and_preds, node); 
            break;
        case AST_BOOL_VARIABLE: 
            match_or_insert(pred_map, &m->var_preds, node); 
            break;
        default:
            switch_default_error("Invalid bool op");
            break;
    }
}

static void assign_set_pred(struct pred_map* pred_map, struct ast_set_expr* typed, struct ast_node* node)
{
    struct pred_set_map* m = &pred_map->set_map;
    switch(typed->op) {
        case AST_SET_NOT_IN: 
            match_or_insert(pred_map, &m->not_in_preds, node); 
            break;
        case AST_SET_IN:
            match_or_insert(pred_map, &m->in_preds, node); 
            break;
        default:
            switch_default_error("Invalid set op");
            break;
    }
}

static void assign_list_pred(struct pred_map* pred_map, struct ast_list_expr* typed, struct ast_node* node)
{
    struct pred_list_map* m = &pred_map->list_map;
    switch(typed->op) {
        case AST_LIST_ALL_OF:
            match_or_insert(pred_map, &m->all_of_preds, node); 
            break;
        case AST_LIST_ONE_OF:
            match_or_insert(pred_map, &m->one_of_preds, node); 
            break;
        case AST_LIST_NONE_OF: 
            match_or_insert(pred_map, &m->none_of_preds, node); 
            break;
        default:
            switch_default_error("Invalid list op");
            break;
    }
}

static void assign_special_pred(struct pred_map* pred_map, struct ast_special_expr* typed, struct ast_node* node)
{
    struct pred_special_map* m = &pred_map->special_map;
    switch(typed->type) {
        case AST_SPECIAL_STRING: 
            match_or_insert(pred_map, &m->string_preds, node); 
            break;
        case AST_SPECIAL_SEGMENT: 
            match_or_insert(pred_map, &m->segment_preds, node); 
            break;
        case AST_SPECIAL_GEO: 
            match_or_insert(pred_map, &m->geo_preds, node); 
            break;
        case AST_SPECIAL_FREQUENCY: 
            match_or_insert(pred_map, &m->frequency_preds, node); 
            break;
        default:
            switch_default_error("Invalid special type");
            break;
    }
}

static void assign_is_null_pred(struct pred_map* pred_map, struct ast_is_null_expr* typed, struct ast_node* node)
{
    struct pred_is_null_map* m = &pred_map->is_null_map;
    switch(typed->type) {
        case AST_IS_NULL:
            match_or_insert(pred_map, &m->is_null_preds, node);
            break;
        case AST_IS_NOT_NULL:
            match_or_insert(pred_map, &m->is_not_null_preds, node);
            break;
        default:
            switch_default_error("Invalid is null type");
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
        case AST_TYPE_COMPARE_EXPR:
            assign_compare_pred(pred_map, &node->compare_expr, node);
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
        case AST_TYPE_IS_NULL_EXPR:
            assign_is_null_pred(pred_map, &node->is_null_expr, node);
            break;
        default:
            switch_default_error("Invalid node type");
            break;
    }
}

static void init_pred_container(struct pred_container* container)
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
    init_pred_container(&pred_map->compare_map.ge_preds);
    init_pred_container(&pred_map->compare_map.gt_preds);
    init_pred_container(&pred_map->compare_map.le_preds);
    init_pred_container(&pred_map->compare_map.lt_preds);
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
    free(pred_map->compare_map.ge_preds.preds);
    free(pred_map->compare_map.gt_preds.preds);
    free(pred_map->compare_map.le_preds.preds);
    free(pred_map->compare_map.lt_preds.preds);
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
    free(pred_map->preds);
    free(pred_map);
}

