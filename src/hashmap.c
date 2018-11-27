#include <stdio.h>

#include "ast.h"
#include "clone.h"
#include "hashmap.h"
#include "map.h"
#include "printer.h"
#include "utils.h"

void assign_pred(struct pred_map* pred_map, struct ast_node* node)
{
    char* str;
    if(node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_NOT) {
        assign_pred(pred_map, node->bool_expr.unary.expr);
        if(asprintf(&str, "not %zu", node->bool_expr.unary.expr->global_id) < 0) {
            abort();
        }
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
        if(asprintf(&str, "or %zu %zu", 
              node->bool_expr.binary.lhs->global_id, 
              node->bool_expr.binary.rhs->global_id) < 0) {
            abort();
        }
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
        if(asprintf(&str, "and %zu %zu", 
              node->bool_expr.binary.lhs->global_id, 
              node->bool_expr.binary.rhs->global_id) < 0) {
            abort();
        }
    }
    else {
        str = ast_to_string(node);
    }
    struct ast_node** in_map = map_get(&pred_map->m, str);
    if(in_map != NULL) {
        struct ast_node* original = *in_map;
        node->global_id = original->global_id;
        if(original->memoize_id == INVALID_PRED) {
            betree_pred_t memoize_id = pred_map->memoize_count;
            pred_map->memoize_count++;
            original->memoize_id = memoize_id;
        }
        node->memoize_id = original->memoize_id;
    }
    else {
        betree_pred_t global_id = pred_map->pred_count;
        pred_map->pred_count++;
        node->global_id = global_id;
        map_set(&pred_map->m, str, node);
    }
    free(str);
}

struct pred_map* make_pred_map()
{
    struct pred_map* pred_map = calloc(1, sizeof(*pred_map));
    if(pred_map == NULL) {
        fprintf(stderr, "%s calloc failed\n", __func__);
        abort();
    }
    pred_map->pred_count = 0;
    map_init(&pred_map->m);
    return pred_map;
}

void free_pred_map(struct pred_map* pred_map)
{
    /*const char *key;*/
    /*map_iter_t iter = map_iter(&pred_map->m);*/

    /*while ((key = map_next(&pred_map->m, &iter))) {*/
        /*free((char*)key);*/
    /*}*/
    map_deinit(&pred_map->m);
    free(pred_map);
}

