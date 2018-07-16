#include <stdio.h>

#include "ast.h"
#include "clone.h"
#include "hashmap.h"
#include "printer.h"
#include "utils.h"

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

static void match_or_insert(struct pred_map* pred_map, struct ast_node* node)
{
    char* key = ast_to_string(node);
    struct htable_ret ret = htable_get(&pred_map->nodes, key);
    if(ret.ok) {
        struct ast_node* other_node = ret.value;
        if(other_node->id == UINT64_MAX) {
            other_node->id = pred_map->pred_count;
            add_predicate_to_map(pred_map, other_node);
        }
        node->id = other_node->id;
    }
    else {
        htable_put(&pred_map->nodes, key, node);
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
    match_or_insert(pred_map, node);
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
    htable_reset(&pred_map->nodes);
    return pred_map;
}

void free_pred_map(struct pred_map* pred_map)
{
    htable_reset(&pred_map->nodes);
    free(pred_map->preds);
    free(pred_map);
}

