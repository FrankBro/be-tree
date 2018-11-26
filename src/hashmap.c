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
        int x = asprintf(&str, "not %zu", node->bool_expr.unary.expr->id);
        (void)x;
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_OR) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
        int x = asprintf(&str, "or %zu %zu", node->bool_expr.binary.lhs->id, node->bool_expr.binary.rhs->id);
        (void)x;
    }
    else if (node->type == AST_TYPE_BOOL_EXPR && node->bool_expr.op == AST_BOOL_AND) {
        assign_pred(pred_map, node->bool_expr.binary.lhs);
        assign_pred(pred_map, node->bool_expr.binary.rhs);
        int x = asprintf(&str, "and %zu %zu", node->bool_expr.binary.lhs->id, node->bool_expr.binary.rhs->id);
        (void)x;
    }
    else {
        str = ast_to_string(node);
    }
    struct ast_node** in_map = map_get(&pred_map->m, str);
    if(in_map != NULL) {
        struct ast_node* original = *in_map;
        node->id = original->id;
        original->use_pred = true;
        node->use_pred = true;
    }
    else {
        betree_pred_t id = pred_map->pred_count;
        pred_map->pred_count++;
        node->id = id;
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

