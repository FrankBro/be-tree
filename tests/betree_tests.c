#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "debug.h"
#include "minunit.h"
#include "utils.h"

struct sub* make_simple_sub1(struct config* config, betree_sub_t id, const char* attr)
{
    struct sub* sub = make_empty_sub(id);
    sub->attr_var_count = 1;
    sub->attr_vars = calloc(1, sizeof(*sub->attr_vars));
    if(sub->attr_vars == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    sub->attr_vars[0] = make_attr_var(attr, config);
    return sub;
}

const struct sub* make_simple_sub_i(
    struct config* config, betree_sub_t id, const char* attr, int64_t ivalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct equality_value value
        = { .value_type = AST_EQUALITY_VALUE_INTEGER, .integer_value = ivalue };
    struct ast_node* expr = ast_equality_expr_create(AST_EQUALITY_EQ, attr, value);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_set_i(
    struct config* config, betree_sub_t id, const char* attr, enum ast_set_e op, int64_t ivalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct set_left_value left
        = { .value_type = AST_SET_LEFT_VALUE_INTEGER, .integer_value = ivalue };
    struct set_right_value right = { .value_type = AST_SET_RIGHT_VALUE_VARIABLE,
        .variable_value = make_attr_var(attr, config) };
    struct ast_node* expr = ast_set_expr_create(op, left, right);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_set_s(struct config* config,
    betree_sub_t id,
    const char* attr,
    enum ast_set_e op,
    struct string_value svalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct set_left_value left
        = { .value_type = AST_SET_LEFT_VALUE_STRING, .string_value = svalue };
    struct set_right_value right = { .value_type = AST_SET_RIGHT_VALUE_VARIABLE,
        .variable_value = make_attr_var(attr, config) };
    struct ast_node* expr = ast_set_expr_create(op, left, right);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_set_il(struct config* config,
    betree_sub_t id,
    const char* attr,
    enum ast_set_e op,
    struct integer_list_value ilvalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct set_left_value left = { .value_type = AST_SET_LEFT_VALUE_VARIABLE,
        .variable_value = make_attr_var(attr, config) };
    struct set_right_value right
        = { .value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST, .integer_list_value = ilvalue };
    struct ast_node* expr = ast_set_expr_create(op, left, right);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_list_il(struct config* config,
    betree_sub_t id,
    const char* attr,
    enum ast_list_e op,
    struct integer_list_value ilvalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct list_value value
        = { .value_type = AST_LIST_VALUE_INTEGER_LIST, .integer_list_value = ilvalue };
    struct ast_node* expr = ast_list_expr_create(op, attr, value);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_list_sl(struct config* config,
    betree_sub_t id,
    const char* attr,
    enum ast_list_e op,
    struct string_list_value slvalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct list_value value
        = { .value_type = AST_LIST_VALUE_STRING_LIST, .string_list_value = slvalue };
    struct ast_node* expr = ast_list_expr_create(op, attr, value);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

struct string_value make_string_value(struct config* config, const char* attr)
{
    struct string_value string_value = { .string = strdup(attr) };
    betree_var_t str_id = get_id_for_string(config, string_value.string);
    string_value.str = str_id;
    return string_value;
}

const struct sub* make_simple_sub_sl(struct config* config,
    betree_sub_t id,
    const char* attr,
    enum ast_set_e op,
    struct string_list_value slvalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct set_left_value left = { .value_type = AST_SET_LEFT_VALUE_VARIABLE,
        .variable_value = make_attr_var(attr, config) };
    struct set_right_value right
        = { .value_type = AST_SET_RIGHT_VALUE_STRING_LIST, .string_list_value = slvalue };
    struct ast_node* expr = ast_set_expr_create(op, left, right);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct sub* make_simple_sub_f(
    struct config* config, betree_sub_t id, const char* attr, double fvalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct equality_value value = { .value_type = AST_EQUALITY_VALUE_FLOAT, .float_value = fvalue };
    struct ast_node* expr = ast_equality_expr_create(AST_EQUALITY_EQ, attr, value);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

// const struct sub* make_simple_sub_b(struct config* config, betree_sub_t id, const char* attr,
// enum ast_bool_e op)
// {
//     struct sub* sub = make_empty_sub(id);
//     sub->variable_id_count = 1;
//     sub->variable_ids = calloc(1, sizeof(*sub->variable_ids));
//     if(sub->variable_ids == NULL) {
//         fprintf(stderr, "%s calloc failed", __func__);
//         abort();
//     }
//     sub->variable_ids[0] = get_id_for_attr(config, attr);
//     struct ast_node* expr = ast_bool_expr_create(op, attr);
//     assign_variable_id(config, expr);
//     sub->expr = expr;
//     return sub;
// }

const struct sub* make_simple_sub_s(
    struct config* config, betree_sub_t id, const char* attr, const char* svalue)
{
    struct sub* sub = make_simple_sub1(config, id, attr);
    struct equality_value value = { .value_type = AST_EQUALITY_VALUE_STRING,
        .string_value = make_string_value(config, svalue) };
    value.string_value.str = get_id_for_string(config, svalue);
    struct ast_node* expr = ast_equality_expr_create(AST_EQUALITY_EQ, attr, value);

    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct event* make_event_with_preds(const size_t size, const struct pred** preds)
{
    struct event* event = calloc(1, sizeof(*event));
    if(event == NULL) {
        fprintf(stderr, "%s event calloc failed", __func__);
        abort();
    }
    event->pred_count = size;
    event->preds = calloc(size, sizeof(*event->preds));
    if(event->preds == NULL) {
        fprintf(stderr, "%s preds calloc failed", __func__);
        abort();
    }
    memcpy(event->preds, preds, sizeof(*event->preds) * size);
    return event;
}

int test_sub_has_attribute()
{
    struct config* config = make_default_config();
    const struct sub* sub = make_simple_sub_i(config, 0, "a", 0);

    mu_assert(sub_has_attribute_str(config, sub, "a"), "Simple sub has 'a'");
    mu_assert(!sub_has_attribute_str(config, sub, "b"), "Simple sub does not have 'b'");

    free_sub((struct sub*)sub);
    free_config(config);
    return 0;
}

int test_remove_sub()
{
    struct config* config = make_default_config();
    const struct sub* sub1 = make_simple_sub_i(config, 0, "a", 0);
    const struct sub* sub2 = make_simple_sub_i(config, 1, "a", 1);
    const struct sub* sub3 = make_simple_sub_i(config, 2, "a", 2);
    struct lnode* lnode = (struct lnode*)make_lnode(config, NULL);

    insert_sub(sub1, lnode);
    mu_assert(lnode->sub_count == 1 && lnode->subs[0] == sub1, "lnode has one sub and it matches");

    remove_sub(sub1, lnode);
    mu_assert(lnode->sub_count == 0, "lnode no longer has any subs");

    insert_sub(sub1, lnode);
    insert_sub(sub2, lnode);
    insert_sub(sub3, lnode);
    mu_assert(lnode->sub_count == 3 && lnode->subs[0] == sub1 && lnode->subs[1] == sub2
            && lnode->subs[2] == sub3,
        "lnode has three subs and they match");

    remove_sub(sub2, lnode);
    mu_assert(lnode->sub_count == 2 && lnode->subs[0] == sub1 && lnode->subs[1] == sub3,
        "lnode no longer has sub2");

    free_lnode(lnode);
    free_sub((struct sub*)sub2);
    free_config(config);
    return 0;
}

void initialize_matched_subs(struct matched_subs* matched_subs)
{
    free(matched_subs->subs);
    matched_subs->sub_count = 0;
    matched_subs->subs = NULL;
}

int test_match_single_cnode()
{
    struct config* config = make_default_config();
    struct cnode* cnode = make_cnode(config, NULL);
    betree_sub_t sub_id = 0;
    struct sub* sub = (struct sub*)make_simple_sub_i(config, sub_id, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub");

    const struct event* goodEvent = make_simple_event_i(config, "a", 0);
    const struct event* wrongValueEvent = make_simple_event_i(config, "a", 1);
    const struct event* wrongVariableEvent = make_simple_event_i(config, "b", 0);

    struct matched_subs* matched_subs = make_matched_subs();
    {
        initialize_matched_subs(matched_subs);
        match_be_tree(config, goodEvent, cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 1 && matched_subs->subs[0] == sub_id, "goodEvent");
    }
    {
        initialize_matched_subs(matched_subs);
        match_be_tree(config, wrongValueEvent, cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 0, "wrongValueEvent");
    }
    {
        // TODO: When we support missing variables/non-missing
        // initialize_matched_subs(&matched_subs);
        // match_be_tree(wrongVariableEvent, cnode, &matchedSub);
        // mu_assert(matched_subs.sub_count == 0, "wrongVariableEvent");
    }

    free_event((struct event*)goodEvent);
    free_event((struct event*)wrongValueEvent);
    free_event((struct event*)wrongVariableEvent);
    free_cnode((struct cnode*)cnode);
    free_config(config);
    free_matched_subs(matched_subs);
    return 0;
}

bool test_attr_in_pnode(struct config* config, const char* attr, const struct pnode* pnode)
{
    if(config == NULL || pnode == NULL || attr == NULL) {
        return false;
    }
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return pnode->attr_var.var == variable_id;
}

bool test_attr_in_cdir(struct config* config, const char* attr, const struct cdir* cdir)
{
    if(config == NULL || cdir == NULL || attr == NULL) {
        return false;
    }
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return cdir->attr_var.var == variable_id;
}

int test_insert_first_split()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub_i(config, 0, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub_i(config, 1, "a", 1);
    struct sub* sub3 = (struct sub*)make_simple_sub_i(config, 2, "a", 2);
    struct sub* sub4 = (struct sub*)make_simple_sub_i(config, 3, "b", 0);
    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub in the first lnode");
    mu_assert(cnode->pdir != NULL, "cnode has a pdir");
    mu_assert(cnode->pdir->pnode_count == 1, "cnode has a pdir with one pnode");
    struct pnode* pnode = cnode->pdir->pnodes[0];
    mu_assert(pnode->cdir != NULL, "pnode has a cdir");
    mu_assert(test_attr_in_pnode(config, "a", pnode), "pnode is for attr 'a'");
    mu_assert(test_attr_in_cdir(config, "a", pnode->cdir), "cdir is for attr 'a'");
    // mu_assert(pnode->cdir->startBound == 0, "startBound");
    // mu_assert(pnode->cdir->endBound == 2, "endBound");
    mu_assert(pnode->cdir->lchild == NULL || pnode->cdir->rchild == NULL,
        "cdir has no lChild and rChild");
    mu_assert(pnode->cdir->cnode != NULL, "cdir has a cnode");
    mu_assert(pnode->cdir->cnode->pdir == NULL, "pdir in the lower cnode does not have a pdir");
    mu_assert(pnode->cdir->cnode->lnode != NULL, "inner cnode has a lnode");
    mu_assert(pnode->cdir->cnode->lnode->sub_count == 3, "tree has three sub in the second lnode");

    free_cnode((struct cnode*)cnode);
    free_config(config);
    return 0;
}

struct ast_node* _AND(const struct ast_node* lhs, const struct ast_node* rhs)
{
    return ast_bool_expr_binary_create(AST_BOOL_AND, lhs, rhs);
}

struct ast_node* ast_numeric_compare_expr_create_i(
    enum ast_numeric_compare_e op, const char* attr, int64_t ivalue)
{
    struct numeric_compare_value value
        = { .value_type = AST_NUMERIC_COMPARE_VALUE_INTEGER, .integer_value = ivalue };
    return ast_numeric_compare_expr_create(op, attr, value);
}

struct ast_node* ast_equality_expr_create_i(
    enum ast_equality_e op, const char* attr, int64_t ivalue)
{
    struct equality_value value
        = { .value_type = AST_EQUALITY_VALUE_INTEGER, .integer_value = ivalue };
    return ast_equality_expr_create(op, attr, value);
}

struct ast_node* _GT(const char* attr, uint64_t value)
{
    return ast_numeric_compare_expr_create_i(AST_NUMERIC_COMPARE_GT, attr, value);
}

struct ast_node* _EQ(const char* attr, uint64_t value)
{
    return ast_equality_expr_create_i(AST_EQUALITY_EQ, attr, value);
}

struct ast_node* _LT(const char* attr, uint64_t value)
{
    return ast_numeric_compare_expr_create_i(AST_NUMERIC_COMPARE_LT, attr, value);
}

bool test_lnode_has_subs(const struct lnode* lnode, size_t sub_count, const struct sub** subs)
{
    if(lnode->sub_count != sub_count) {
        return false;
    }
    for(size_t i = 0; i < sub_count; i++) {
        if(lnode->subs[i] != subs[i]) {
            return false;
        }
    }
    return true;
}

bool test_cnode_has_pnodes(
    struct config* config, const struct cnode* cnode, size_t pnode_count, const char** attrs)
{
    if(cnode->pdir == NULL || cnode->pdir->pnode_count != pnode_count) {
        return false;
    }
    for(size_t i = 0; i < pnode_count; i++) {
        if(!test_attr_in_pnode(config, attrs[i], cnode->pdir->pnodes[i])) {
            return false;
        }
    }
    return true;
}

int test_pdir_split_twice()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    add_attr_domain_i(config, "c", 0, 10, false);
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub_i(config, 1, "a", 0);
    const struct sub* sub2 = make_simple_sub_i(config, 2, "a", 0);
    const struct sub* sub3 = make_simple_sub_i(config, 3, "a", 0);
    const struct sub* sub4 = make_simple_sub_i(config, 4, "b", 0);
    const struct sub* sub5 = make_simple_sub_i(config, 5, "b", 0);
    const struct sub* sub6 = make_simple_sub_i(config, 6, "b", 0);
    const struct sub* sub7 = make_simple_sub_i(config, 7, "c", 0);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs456[3] = { sub4, sub5, sub6 };
    const struct sub* subs7[1] = { sub7 };

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->lnode, 3, subs123), "subs123 in first lnode");

    insert_be_tree(config, sub4, cnode, NULL);
    insert_be_tree(config, sub5, cnode, NULL);
    insert_be_tree(config, sub6, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123),
        "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 3, subs456), "subs456 in first lnode");

    insert_be_tree(config, sub7, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123),
        "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[1]->cdir->cnode->lnode, 3, subs456),
        "subs456 in third lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs7), "subs7 in first lnode");

    free_cnode((struct cnode*)cnode);
    free_config(config);
    return 0;
}

int test_cdir_split_twice()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub_i(config, 1, "a", 2);
    const struct sub* sub2 = make_simple_sub_i(config, 2, "a", 2);
    const struct sub* sub3 = make_simple_sub_i(config, 3, "a", 2);
    const struct sub* sub4 = make_simple_sub_i(config, 4, "b", 0);
    const struct sub* sub5 = make_simple_sub_i(config, 5, "a", 7);
    const struct sub* sub6 = make_simple_sub_i(config, 6, "a", 7);
    const struct sub* sub7 = make_simple_sub_i(config, 7, "a", 7);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs4[1] = { sub4 };
    const struct sub* subs567[3] = { sub5, sub6, sub7 };

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->lnode, 3, subs123), "subs123 in first lnode");

    insert_be_tree(config, sub4, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123),
        "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "subs4 in first lnode");

    insert_be_tree(config, sub5, cnode, NULL);
    insert_be_tree(config, sub6, cnode, NULL);
    insert_be_tree(config, sub7, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode, 3, subs123),
        "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode, 3, subs567),
        "subs567 in third lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "subs4 in first lnode");

    free_cnode((struct cnode*)cnode);
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree()
{
    struct config* config = make_default_config();
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub_i(config, 0, "a", 0);

    insert_be_tree(config, sub1, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "lnode has the sub");

    delete_be_tree(config, sub1, cnode);

    mu_assert(cnode->lnode->sub_count == 0, "lnode does not have the sub");
    mu_assert(cnode != NULL && cnode->lnode != NULL,
        "did not delete the cnode or lnode because it's root");

    free_cnode(cnode);
    free_sub(sub1);
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree_with_delete()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub_i(config, 1, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub_i(config, 2, "a", 0);
    struct sub* sub3 = (struct sub*)make_simple_sub_i(config, 3, "a", 0);
    struct sub* sub4 = (struct sub*)make_simple_sub_i(config, 4, "b", 0);

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs4[1] = { sub4 };
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "sub 4 is in lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123),
        "sub 1, 2, and 3 is lower lnode");

    delete_be_tree(config, sub1, cnode);
    delete_be_tree(config, sub2, cnode);
    delete_be_tree(config, sub3, cnode);

    mu_assert(cnode->pdir == NULL, "deleted everything down of the pdir");

    free_cnode(cnode);
    free_sub(sub1);
    free_sub(sub2);
    free_sub(sub3);
    free_config(config);
    return 0;
}

int test_match_deeper()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 0, false);
    add_attr_domain_i(config, "b", 0, 1, false);

    struct cnode* cnode = make_cnode(config, NULL);
    const struct sub* sub1 = make_sub(config, 1, _AND(_EQ("a", 0), _EQ("b", 0)));
    const struct sub* sub2 = make_simple_sub_i(config, 2, "a", 1);
    const struct sub* sub3 = make_sub(config, 3, _AND(_EQ("a", 0), _EQ("b", 0)));
    const struct sub* sub4 = make_sub(config, 4, _AND(_EQ("a", 0), _EQ("b", 1)));

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    const struct lnode* lnode = cnode->lnode;
    const struct pdir* pdir_a = cnode->pdir;
    const struct pnode* pnode_a = pdir_a->pnodes[0];
    const struct cdir* cdir_a = pnode_a->cdir;
    const struct cnode* cnode_a = cdir_a->cnode;
    const struct lnode* lnode_a = cnode_a->lnode;
    const struct pdir* pdir_b = cnode_a->pdir;
    const struct pnode* pnode_b = pdir_b->pnodes[0];
    const struct cdir* cdir_b = pnode_b->cdir;
    const struct cnode* cnode_b = cdir_b->cnode;
    const struct lnode* lnode_b = cnode_b->lnode;

    mu_assert(lnode->sub_count == 0 && pdir_a->pnode_count == 1
            && test_attr_in_pnode(config, "a", pnode_a) && test_attr_in_cdir(config, "a", cdir_a)
            && lnode_a->sub_count == 1 && pdir_b->pnode_count == 1
            && test_attr_in_pnode(config, "b", pnode_b) && test_attr_in_cdir(config, "b", cdir_b)
            && cdir_b->lchild == NULL && cdir_b->rchild == NULL && lnode_b->sub_count == 3,
        "tree matches what we expected");

    mu_assert(lnode_a->sub_count == 1, "lnode in 'a' has one sub");
    mu_assert(lnode_b->sub_count == 3, "lnode in 'a' has one sub");

    const struct pred* pred_a = make_simple_pred_str_i(config, "a", 0);
    const struct pred* pred_b = make_simple_pred_str_i(config, "b", 1);
    const struct pred* preds[2] = { pred_a, pred_b };
    const struct event* event = make_event_with_preds(2, preds);

    struct matched_subs* matched_subs = make_matched_subs();
    initialize_matched_subs(matched_subs);
    match_be_tree(config, event, cnode, matched_subs, NULL);
    mu_assert(matched_subs->sub_count == 1 && matched_subs->subs[0] == 4, "goodEvent");

    free_event((struct event*)event);
    free_cnode((struct cnode*)cnode);
    free_matched_subs(matched_subs);
    free_config(config);
    return 0;
}

int test_large_cdir_split()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10000, false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 100; i++) {
        const struct sub* sub = make_simple_sub_i(config, i, "a", i);
        insert_be_tree(config, sub, cnode, NULL);
    }

    const struct event* event = make_simple_event_i(config, "a", 0);
    struct matched_subs* matched_subs = make_matched_subs();
    match_be_tree(config, event, cnode, matched_subs, NULL);

    mu_assert(matched_subs->sub_count == 1, "matched one");

    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
    free_config(config);
    return 0;
}

int test_min_partition()
{
    size_t lnode_max_cap = 3;
    // With 0
    struct config* config = make_config(lnode_max_cap, 0);
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    add_attr_domain_i(config, "c", 0, 10, false);

    struct cnode* cnode = make_cnode(config, NULL);

    struct sub* sub = NULL;
    sub = (struct sub*)make_simple_sub_i(config, 0, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 1, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 2, "b", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 3, "c", 0);
    insert_be_tree(config, sub, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 2, "First lnode has two subs");
    mu_assert(cnode->pdir->pnode_count == 1 && cnode->pdir->pnodes[0]->attr_var.var == 0
            && cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 2,
        "Has a pnode for 'a' and two subs");

    free_cnode(cnode);
    free_config(config);

    // With 3
    config = make_config(lnode_max_cap, 3);
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    add_attr_domain_i(config, "c", 0, 10, false);

    cnode = make_cnode(config, NULL);

    sub = (struct sub*)make_simple_sub_i(config, 0, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 1, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 2, "b", 0);
    insert_be_tree(config, sub, cnode, NULL);
    sub = (struct sub*)make_simple_sub_i(config, 3, "c", 0);
    insert_be_tree(config, sub, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 4, "First lnode has four subs");
    mu_assert(cnode->lnode->max != lnode_max_cap, "First lnode max cap went up");

    free_cnode(cnode);
    free_config(config);

    return 0;
}

int parse(const char* text, struct ast_node** node);

int test_allow_undefined()
{
    enum { expr_count = 4 };
    const char* exprs[expr_count] = { "a = 0", "a = 1", "a = 0 || b = 0", "b = 1" };

    // With defined only
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);

    struct cnode* cnode = make_cnode(config, NULL);
    struct event* event = (struct event*)make_simple_event_i(config, "b", 0);

    for(size_t i = 0; i < expr_count; i++) {
        const char* expr = exprs[i];
        struct ast_node* node = NULL;
        (void)parse(expr, &node);
        const struct sub* sub = make_sub(config, i + 1, node);
        insert_be_tree(config, sub, cnode, NULL);
    }

    struct matched_subs* matched_subs = make_matched_subs();
    match_be_tree(config, event, cnode, matched_subs, NULL);

    mu_assert(cnode->lnode->sub_count == 1 && cnode->pdir != NULL && cnode->pdir->pnode_count == 1
            && cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "Structure is what is expected");
    mu_assert(matched_subs->sub_count == 0, "Found no sub");

    free_cnode(cnode);
    free_config(config);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);

    // With allow undefined
    config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, true);
    add_attr_domain_i(config, "b", 0, 10, false);

    cnode = make_cnode(config, NULL);
    event = (struct event*)make_simple_event_i(config, "b", 0);

    for(size_t i = 0; i < expr_count; i++) {
        const char* expr = exprs[i];
        struct ast_node* node = NULL;
        (void)parse(expr, &node);
        const struct sub* sub = make_sub(config, i + 1, node);
        insert_be_tree(config, sub, cnode, NULL);
    }

    matched_subs = make_matched_subs();
    match_be_tree(config, event, cnode, matched_subs, NULL);

    mu_assert(cnode->lnode->sub_count == 1 && cnode->pdir != NULL && cnode->pdir->pnode_count == 1
            && cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "Structure is what is expected");
    mu_assert(matched_subs->sub_count == 1, "Found the sub in the lower lnode");

    free_cnode(cnode);
    free_config(config);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);

    return 0;
}

int test_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "a", 0., 10., false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? 0. : 7.;
        struct sub* sub = (struct sub*)make_simple_sub_f(config, i, "a", value);
        insert_be_tree(config, sub, cnode, NULL);
    }

    const struct pnode* pnode = cnode->pdir->pnodes[0];
    mu_assert(cnode->pdir != NULL && cnode->pdir->pnode_count == 1 && pnode->cdir != NULL
            && pnode->cdir->cnode->lnode->sub_count == 0 && pnode->cdir->lchild != NULL
            && pnode->cdir->rchild != NULL && pnode->cdir->lchild->cnode->lnode->sub_count == 3
            && pnode->cdir->rchild->cnode->lnode->sub_count == 1,
        "structure is respected");

    free_cnode(cnode);
    free_config(config);

    return 0;
}

// int test_bool()
// {
//     struct config* config = make_default_config();
//     add_attr_domain_b(config, "a", false, true, false);

//     struct cnode* cnode = make_cnode(config, NULL);

//     for(size_t i = 0; i < 4; i++) {
//         enum ast_bool_e op = i < 3 ? AST_BOOL_NOT : AST_BOOL_NONE;
//         struct sub* sub = (struct sub*)make_simple_sub_b(config, i, "a", op);
//         insert_be_tree(config, sub, cnode, NULL);
//     }

//     const struct pnode* pnode = cnode->pdir->pnodes[0];
//     mu_assert(cnode->pdir != NULL &&
//         cnode->pdir->pnode_count == 1 &&
//         pnode->cdir != NULL &&
//         pnode->cdir->cnode->lnode->sub_count == 0 &&
//         pnode->cdir->lchild != NULL &&
//         pnode->cdir->rchild != NULL &&
//         pnode->cdir->lchild->cnode->lnode->sub_count == 3 &&
//         pnode->cdir->rchild->cnode->lnode->sub_count == 1
//     , "structure is respected");

//     free_cnode(cnode);
//     free_config(config);

//     return 0;
// }

int test_string()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "a", false);

    struct cnode* cnode = make_cnode(config, NULL);

    struct sub* sub = (struct sub*)make_simple_sub_s(config, 0, "a", "a");
    insert_be_tree(config, sub, cnode, NULL);

    const struct event* event = make_simple_event_s(config, "a", "a");
    struct matched_subs* matched_subs = make_matched_subs();
    match_be_tree(config, event, cnode, matched_subs, NULL);

    mu_assert(matched_subs->sub_count == 1, "found our sub");

    free_cnode(cnode);
    free_config(config);
    free_matched_subs(matched_subs);
    free_event((struct event*)event);

    return 0;
}

int test_string_wont_split()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "a", false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        struct sub* sub = (struct sub*)make_simple_sub_s(config, i, "a", "a");
        insert_be_tree(config, sub, cnode, NULL);
    }

    mu_assert(cnode->lnode->sub_count == 4, "did not split");

    free_cnode(cnode);
    free_config(config);

    return 0;
}

int test_negative_int()
{
    struct config* config = make_default_config();
    int64_t min = -10;
    int64_t max = -4;
    int64_t mid = (min + max) / 2;
    add_attr_domain_i(config, "a", min, max, false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        int64_t value = i < 3 ? -6 : -12;
        struct sub* sub = (struct sub*)make_simple_sub_i(config, i, "a", value);
        insert_be_tree(config, sub, cnode, NULL);
    }

    const struct cdir* cdir = cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == VALUE_I && cdir->bound.imin == min
            && cdir->bound.imax == max && lchild->bound.value_type == VALUE_I
            && lchild->bound.imin == min && lchild->bound.imax == mid
            && rchild->bound.value_type == VALUE_I && rchild->bound.imin == mid
            && rchild->bound.imax == max,
        "cdirs have proper bounds");

    free_config(config);
    free_cnode(cnode);

    return 0;
}

int test_negative_float()
{
    struct config* config = make_default_config();
    double min = -10.;
    double max = -4.;
    double mid = ceil((min + max) / 2.);
    add_attr_domain_f(config, "a", min, max, false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        int64_t value = i < 3 ? -6. : -12.;
        struct sub* sub = (struct sub*)make_simple_sub_f(config, i, "a", value);
        insert_be_tree(config, sub, cnode, NULL);
    }

    const struct cdir* cdir = cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == VALUE_F && feq(cdir->bound.fmin, min)
            && feq(cdir->bound.fmax, max) && lchild->bound.value_type == VALUE_F
            && feq(lchild->bound.fmin, min) && feq(lchild->bound.fmax, mid)
            && rchild->bound.value_type == VALUE_F && feq(rchild->bound.fmin, mid)
            && feq(rchild->bound.fmax, max),
        "cdirs have proper bounds");

    free_config(config);
    free_cnode(cnode);

    return 0;
}

int test_integer_set()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_set_il(config, 0, "a", AST_SET_IN, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        const struct event* event = make_simple_event_i(config, "a", 0);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_set_il(config, 0, "a", AST_SET_NOT_IN, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        const struct event* event = make_simple_event_i(config, "a", 0);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_integer_set_reverse()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "a", false);

    size_t count = 3;
    struct integer_list_value integer_list = { .count = count };
    integer_list.integers = calloc(3, sizeof(*integer_list.integers));
    integer_list.integers[0] = 1;
    integer_list.integers[1] = 2;
    integer_list.integers[2] = 0;
    const struct event* event = make_simple_event_il(config, "a", integer_list);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        struct sub* sub = (struct sub*)make_simple_sub_set_i(config, 0, "a", AST_SET_IN, 0);
        insert_be_tree(config, sub, cnode, NULL);

        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        struct sub* sub = (struct sub*)make_simple_sub_set_i(config, 0, "a", AST_SET_NOT_IN, 0);
        insert_be_tree(config, sub, cnode, NULL);

        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_event((struct event*)event);
    free_config(config);

    return 0;
}

int test_string_set()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "a", false);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "b");
        string_list.strings[1] = make_string_value(config, "c");
        string_list.strings[2] = make_string_value(config, "a");

        struct sub* sub = (struct sub*)make_simple_sub_sl(config, 0, "a", AST_SET_IN, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        const struct event* event = make_simple_event_s(config, "a", "a");
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "b");
        string_list.strings[1] = make_string_value(config, "c");
        string_list.strings[2] = make_string_value(config, "a");

        struct sub* sub
            = (struct sub*)make_simple_sub_sl(config, 0, "a", AST_SET_NOT_IN, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        const struct event* event = make_simple_event_s(config, "a", "a");
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_string_set_reverse()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "a", false);

    size_t count = 3;
    struct string_list_value string_list = { .count = count };
    string_list.strings = calloc(3, sizeof(*string_list.strings));
    string_list.strings[0] = make_string_value(config, "1");
    string_list.strings[1] = make_string_value(config, "2");
    string_list.strings[2] = make_string_value(config, "0");
    const struct event* event = make_simple_event_sl(config, "a", string_list);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        struct string_value value = make_string_value(config, "0");
        struct sub* sub = (struct sub*)make_simple_sub_set_s(config, 0, "a", AST_SET_IN, value);
        insert_be_tree(config, sub, cnode, NULL);

        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        struct string_value value = make_string_value(config, "0");
        struct sub* sub = (struct sub*)make_simple_sub_set_s(config, 0, "a", AST_SET_NOT_IN, value);
        insert_be_tree(config, sub, cnode, NULL);

        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_event((struct event*)event);
    free_config(config);

    return 0;
}

int test_integer_list()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "a", false);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_ONE_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 1;
        event_integer_list.integers[1] = 2;
        event_integer_list.integers[2] = 0;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_ONE_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 4;
        event_integer_list.integers[1] = 5;
        event_integer_list.integers[2] = 3;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_NONE_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 4;
        event_integer_list.integers[1] = 5;
        event_integer_list.integers[2] = 3;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_NONE_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 1;
        event_integer_list.integers[1] = 2;
        event_integer_list.integers[2] = 0;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_ALL_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 1;
        event_integer_list.integers[1] = 2;
        event_integer_list.integers[2] = 0;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct integer_list_value integer_list = { .count = count };
        integer_list.integers = calloc(3, sizeof(*integer_list.integers));
        integer_list.integers[0] = 1;
        integer_list.integers[1] = 2;
        integer_list.integers[2] = 0;

        struct sub* sub
            = (struct sub*)make_simple_sub_list_il(config, 0, "a", AST_LIST_ALL_OF, integer_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct integer_list_value event_integer_list = { .count = event_count };
        event_integer_list.integers = calloc(3, sizeof(*event_integer_list.integers));
        event_integer_list.integers[0] = 1;
        event_integer_list.integers[1] = 2;
        event_integer_list.integers[2] = 3;

        const struct event* event = make_simple_event_il(config, "a", event_integer_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_string_list()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "a", false);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_ONE_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "1");
        event_string_list.strings[1] = make_string_value(config, "2");
        event_string_list.strings[2] = make_string_value(config, "0");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_ONE_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "4");
        event_string_list.strings[1] = make_string_value(config, "5");
        event_string_list.strings[2] = make_string_value(config, "3");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_NONE_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "4");
        event_string_list.strings[1] = make_string_value(config, "5");
        event_string_list.strings[2] = make_string_value(config, "3");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_NONE_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "1");
        event_string_list.strings[1] = make_string_value(config, "2");
        event_string_list.strings[2] = make_string_value(config, "0");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_ALL_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "1");
        event_string_list.strings[1] = make_string_value(config, "2");
        event_string_list.strings[2] = make_string_value(config, "0");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        size_t count = 3;
        struct string_list_value string_list = { .count = count };
        string_list.strings = calloc(3, sizeof(*string_list.strings));
        string_list.strings[0] = make_string_value(config, "1");
        string_list.strings[1] = make_string_value(config, "2");
        string_list.strings[2] = make_string_value(config, "0");

        struct sub* sub
            = (struct sub*)make_simple_sub_list_sl(config, 0, "a", AST_LIST_ALL_OF, string_list);
        insert_be_tree(config, sub, cnode, NULL);

        size_t event_count = 3;
        struct string_list_value event_string_list = { .count = event_count };
        event_string_list.strings = calloc(3, sizeof(*event_string_list.strings));
        event_string_list.strings[0] = make_string_value(config, "1");
        event_string_list.strings[1] = make_string_value(config, "2");
        event_string_list.strings[2] = make_string_value(config, "3");

        const struct event* event = make_simple_event_sl(config, "a", event_string_list);
        struct matched_subs* matched_subs = make_matched_subs();
        match_be_tree(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_event((struct event*)event);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int event_parse(const char* text, struct event** event);

int test_parenthesis()
{
    struct config* config = make_default_config();
    add_attr_domain_b(config, "a", false, true, false);
    add_attr_domain_b(config, "b", false, true, false);
    add_attr_domain_b(config, "c", false, true, false);

    {
        struct cnode* cnode = make_cnode(config, NULL);
        betree_insert(config, 1, "a || (b && c)", cnode);
        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\":true,\"b\":false,\"c\":false}", cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 1, "");
        free_cnode(cnode);
        free_matched_subs(matched_subs);
    }
    {
        struct cnode* cnode = make_cnode(config, NULL);
        betree_insert(config, 1, "(a || b) && c", cnode);
        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\":true,\"b\":false,\"c\":false}", cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 0, "");
        free_cnode(cnode);
        free_matched_subs(matched_subs);
    }
    {
        struct cnode* cnode = make_cnode(config, NULL);
        betree_insert(config, 1, "a || (b && c)", cnode);
        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\":false,\"b\":true,\"c\":true}", cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 1, "");
        free_cnode(cnode);
        free_matched_subs(matched_subs);
    }

    free_config(config);

    return 0;
}

int all_tests()
{
    mu_run_test(test_sub_has_attribute);
    mu_run_test(test_remove_sub);
    mu_run_test(test_match_single_cnode);
    mu_run_test(test_insert_first_split);
    mu_run_test(test_pdir_split_twice);
    mu_run_test(test_cdir_split_twice);
    mu_run_test(test_remove_sub_in_tree);
    mu_run_test(test_remove_sub_in_tree_with_delete);
    mu_run_test(test_match_deeper);
    mu_run_test(test_large_cdir_split);
    mu_run_test(test_min_partition);
    mu_run_test(test_allow_undefined);
    mu_run_test(test_float);
    // mu_run_test(test_bool);
    mu_run_test(test_string);
    mu_run_test(test_string_wont_split);
    mu_run_test(test_negative_int);
    mu_run_test(test_negative_float);
    mu_run_test(test_integer_set);
    mu_run_test(test_integer_set_reverse);
    mu_run_test(test_string_set);
    mu_run_test(test_string_set_reverse);
    mu_run_test(test_integer_list);
    mu_run_test(test_string_list);
    mu_run_test(test_parenthesis);

    return 0;
}

RUN_TESTS()
