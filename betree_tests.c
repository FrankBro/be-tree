#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "minunit.h"

#define SEP "------------------------------------------------------"

void print_lnode(const struct lnode* lnode, unsigned int level)
{
    if(lnode == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" lnode (%d) [", lnode->max);
    for(unsigned int i = 0; i < lnode->sub_count; i++) {
        printf("%d", lnode->subs[i]->id);
        if(i != lnode->sub_count - 1) {
            printf(", ");
        }
    }
    printf("]");
}

void print_cnode(const struct cnode* cnode, unsigned int level);

void print_cdir(const struct cdir* cdir, unsigned int level)
{
    if(cdir == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" cdir [%d, %d]", cdir->startBound, cdir->endBound);
    if(cdir->cnode != NULL) {
        printf("\n");
        print_cnode(cdir->cnode, level + 1);
    }
    if(cdir->lChild != NULL) {
        printf("\n");
        print_cdir(cdir->lChild, level + 1);
    }
    if(cdir->rChild != NULL) {
        printf("\n");
        print_cdir(cdir->rChild, level + 1);
    }
}

void print_pnode(const struct pnode* pnode, unsigned int level)
{
    if(pnode == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" pnode %s (%f)", pnode->attr, pnode->score);
    if(pnode->cdir != NULL) {
        printf("\n");
        print_cdir(pnode->cdir, level + 1);
    }
}

void print_pdir(const struct pdir* pdir, unsigned int level)
{
    if(pdir == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" pdir");
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        if(pnode != NULL) {
            printf("\n");
            print_pnode(pnode, level + 1);
        }
    }
}

void print_cnode(const struct cnode* cnode, unsigned int level)
{
    if(cnode == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" cnode");
    if(cnode->lnode != NULL) {
        printf("\n");
        print_lnode(cnode->lnode, level + 1);
    }
    if(cnode->pdir != NULL) {
        printf("\n");
        print_pdir(cnode->pdir, level + 1);
    }
}

void print_be_tree(const struct cnode* root)
{
    print_cnode(root, 0);
}

const struct pred* make_simple_pred(const char* attr, int value)
{
    struct pred* pred = malloc(sizeof(struct pred));
    pred->attr = strdup(attr);
    pred->value = value;
    return pred;
}

struct sub* make_empty_sub(int id)
{
    struct sub* sub = malloc(sizeof(struct sub));
    sub->id = id;
    sub->pred_count = 0;
    sub->preds = NULL;
    return sub;
}

const struct sub* make_simple_sub(int id, const char* attr, int value)
{
    struct sub* sub = make_empty_sub(id);
    sub->pred_count = 1;
    sub->preds = malloc(sizeof(struct pred));
    sub->preds[0] = make_simple_pred(attr, value);
    const struct ast_node* expr = ast_binary_expr_create(BINOP_EQ, attr, value);
    sub->expr = expr;
    return sub;
}

const struct event* make_simple_event(const char* attr, int value)
{
    struct event* event = malloc(sizeof(struct event));
    event->pred_count = 1;
    event->preds = malloc(sizeof(struct pred));
    event->preds[0] = make_simple_pred(attr, value);
    return event;
}

struct config* config = NULL;

int test_sub_has_attribute() 
{
    const struct sub* sub = make_simple_sub(0, "a", 0);

    mu_assert(sub_has_attribute(sub, "a"), "Simple sub has 'a'");
    mu_assert(!sub_has_attribute(sub, "b"), "Simple sub does not have 'b'");

    free_sub((struct sub*)sub);
    return 0;
}

int test_remove_sub()
{
    const struct sub* sub1 = make_simple_sub(0, "a", 0);
    const struct sub* sub2 = make_simple_sub(1, "a", 1);
    const struct sub* sub3 = make_simple_sub(2, "a", 2);
    struct lnode* lnode = (struct lnode*)make_lnode(config, NULL);

    insert_sub(sub1, lnode);
    mu_assert(lnode->sub_count == 1 && lnode->subs[0] == sub1, "lnode has one sub and it matches");

    remove_sub(sub1, lnode);
    mu_assert(lnode->sub_count == 0, "lnode no longer has any subs");

    insert_sub(sub1, lnode);
    insert_sub(sub2, lnode);
    insert_sub(sub3, lnode);
    mu_assert(lnode->sub_count == 3 && lnode->subs[0] == sub1 && lnode->subs[1] == sub2 && lnode->subs[2] == sub3, "lnode has three subs and they match");

    remove_sub(sub2, lnode);
    mu_assert(lnode->sub_count == 2 && lnode->subs[0] == sub1 && lnode->subs[1] == sub3, "lnode no longer has sub2");

    free_lnode(lnode);
    free_sub((struct sub*)sub2);
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
    struct cnode* cnode = make_cnode(config, NULL);
    int subId = 0;
    struct sub* sub = (struct sub*)make_simple_sub(subId, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub");

    const struct event* goodEvent = make_simple_event("a", 0);
    const struct event* wrongValueEvent = make_simple_event("a", 1);
    const struct event* wrongVariableEvent = make_simple_event("b", 0);

    struct matched_subs matched_subs = { .sub_count = 0, .subs = NULL };
    {
        initialize_matched_subs(&matched_subs);
        match_be_tree(goodEvent, cnode, &matched_subs);
        mu_assert(matched_subs.sub_count == 1 && matched_subs.subs[0] == subId, "goodEvent");
    }
    {
        initialize_matched_subs(&matched_subs);
        match_be_tree(wrongValueEvent, cnode, &matched_subs);
        mu_assert(matched_subs.sub_count == 0, "wrongValueEvent");
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
    return 0;
}

int test_insert_first_split()
{
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(0, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub(1, "a", 1);
    struct sub* sub3 = (struct sub*)make_simple_sub(2, "a", 2);
    struct sub* sub4 = (struct sub*)make_simple_sub(3, "b", 0);
    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub in the first lnode");
    mu_assert(cnode->pdir != NULL, "cnode has a pdir");
    mu_assert(cnode->pdir->pnode_count == 1, "cnode has a pdir with one pnode");
    struct pnode* pnode = cnode->pdir->pnodes[0];
    mu_assert(pnode->cdir != NULL, "pnode has a cdir");
    mu_assert(strcasecmp(pnode->attr, "a") == 0, "pnode is for attr 'a'");
    mu_assert(strcasecmp(pnode->cdir->attr, "a") == 0, "cdir is for attr 'a'");
    // mu_assert(pnode->cdir->startBound == 0, "startBound");
    // mu_assert(pnode->cdir->endBound == 2, "endBound");
    mu_assert(pnode->cdir->lChild == NULL || pnode->cdir->rChild == NULL, "cdir has no lChild and rChild");
    mu_assert(pnode->cdir->cnode != NULL, "cdir has a cnode");
    mu_assert(pnode->cdir->cnode->pdir == NULL, "pdir in the lower cnode does not have a pdir");
    mu_assert(pnode->cdir->cnode->lnode != NULL, "inner cnode has a lnode");
    mu_assert(pnode->cdir->cnode->lnode->sub_count == 3, "tree has three sub in the second lnode");

    free_cnode((struct cnode*)cnode);
    return 0;
}

const struct ast_node* _AND (const struct ast_node* lhs, const struct ast_node* rhs)
{
    return ast_combi_expr_create(COMBI_AND, lhs, rhs);
}

const struct ast_node* _GT (const char* attr, int value)
{
    return ast_binary_expr_create(BINOP_GT, attr, value);
}

const struct ast_node* _EQ (const char* attr, int value)
{
    return ast_binary_expr_create(BINOP_EQ, attr, value);
}

const struct ast_node* _LT (const char* attr, int value)
{
    return ast_binary_expr_create(BINOP_LT, attr, value);
}

void fill_pred(struct sub* sub, const struct ast_node* expr)
{
    switch(expr->type) {
        case AST_TYPE_COMBI_EXPR: {
            fill_pred(sub, expr->combi_expr.lhs);
            fill_pred(sub, expr->combi_expr.rhs);
            return;
        }
        case AST_TYPE_BINARY_EXPR: {
            bool is_found = false;
            for(unsigned int i = 0; i < sub->pred_count; i++) {
                if(strcasecmp(sub->preds[i]->attr, expr->binary_expr.name) == 0) {
                    is_found = true;
                    break;
                }
            }
            if(!is_found) {
                sub->preds[sub->pred_count] = make_simple_pred(expr->binary_expr.name, 0);
                sub->pred_count++;
            }
        }
    }
}

const struct sub* make_complex_sub(int* id_gen, const struct ast_node* expr)
{
    int id = *id_gen;
    (*id_gen)++;
    struct sub* sub = make_empty_sub(id);
    sub->expr = expr;
    fill_pred(sub, sub->expr);
    return sub;
}

bool test_lnode_has_subs(const struct lnode* lnode, unsigned int sub_count, const struct sub** subs)
{
    if(lnode->sub_count != sub_count) {
        return false;
    }
    for(unsigned int i = 0; i < sub_count; i++) {
        if(lnode->subs[i] != subs[i]) {
            return false;
        }
    }
    return true;
} 

bool test_cnode_has_pnodes(const struct cnode* cnode, unsigned int pnode_count, const char** attrs)
{
    if(cnode->pdir == NULL || cnode->pdir->pnode_count != pnode_count) {
        return false;
    }
    for(unsigned int i = 0; i < pnode_count; i++) {
        if(strcasecmp(cnode->pdir->pnodes[i]->attr, attrs[i]) != 0) {
            return false;
        }
    }
    return true;
}

int test_pdir_split_twice()
{
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub(1, "a", 0);
    const struct sub* sub2 = make_simple_sub(2, "a", 0);
    const struct sub* sub3 = make_simple_sub(3, "a", 0);
    const struct sub* sub4 = make_simple_sub(4, "b", 0);
    const struct sub* sub5 = make_simple_sub(5, "b", 0);
    const struct sub* sub6 = make_simple_sub(6, "b", 0);
    const struct sub* sub7 = make_simple_sub(7, "c", 0);

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

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123), "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 3, subs456), "subs456 in first lnode");

    insert_be_tree(config, sub7, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123), "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[1]->cdir->cnode->lnode, 3, subs456), "subs456 in third lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs7), "subs7 in first lnode");

    free_cnode((struct cnode*)cnode);
    return 0;
}

int test_cdir_split_twice()
{
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub(1, "a", 2);
    const struct sub* sub2 = make_simple_sub(2, "a", 2);
    const struct sub* sub3 = make_simple_sub(3, "a", 2);
    const struct sub* sub4 = make_simple_sub(4, "b", 0);
    const struct sub* sub5 = make_simple_sub(5, "a", 7);
    const struct sub* sub6 = make_simple_sub(6, "a", 7);
    const struct sub* sub7 = make_simple_sub(7, "a", 7);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs4[1] = { sub4 };
    const struct sub* subs567[3] = { sub5, sub6, sub7 };

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->lnode, 3, subs123), "subs123 in first lnode");

    insert_be_tree(config, sub4, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123), "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "subs4 in first lnode");

    insert_be_tree(config, sub5, cnode, NULL);
    insert_be_tree(config, sub6, cnode, NULL);
    insert_be_tree(config, sub7, cnode, NULL);

    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->lChild->cnode->lnode, 3, subs123), "subs123 in second lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->rChild->cnode->lnode, 3, subs567), "subs567 in third lnode");
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "subs4 in first lnode");

    free_cnode((struct cnode*)cnode);
    return 0;
}

int test_remove_sub_in_tree()
{
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(0, "a", 0);

    insert_be_tree(config, sub1, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "lnode has the sub");

    delete_be_tree(config, sub1, cnode);

    mu_assert(cnode->lnode->sub_count == 0, "lnode does not have the sub");
    mu_assert(cnode != NULL && cnode->lnode != NULL, "did not delete the cnode or lnode because it's root");

    free_cnode(cnode);
    free_sub(sub1);
    return 0;
}

int test_remove_sub_in_tree_with_delete()
{
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(1, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub(2, "a", 0);
    struct sub* sub3 = (struct sub*)make_simple_sub(3, "a", 0);
    struct sub* sub4 = (struct sub*)make_simple_sub(4, "b", 0);

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs4[1] = { sub4 };
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "sub 4 is in lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123), "sub 1, 2, and 3 is lower lnode");

    delete_be_tree(config, sub1, cnode);
    // print_be_tree(cnode);
    delete_be_tree(config, sub2, cnode);
    // print_be_tree(cnode);
    delete_be_tree(config, sub3, cnode);

    mu_assert(cnode->pdir == NULL, "deleted everything down of the pdir");

    free_cnode(cnode);
    free_sub(sub1);
    free_sub(sub2);
    free_sub(sub3);
    return 0;
}

// int test_remove_sub_in_tree_with_two_levels_pnode()
// {

// }

// int test_remove_sub_in_tree_with_two_levels_cdir()
// {

// }

int test_match_deeper()
{
    struct config local_config = *config;
    struct attr_domain attr_domain_a = { .name = "a", .minBound = 0, .maxBound = 1 };
    local_config.attr_domains[0] = attr_domain_a;
    struct attr_domain attr_domain_b = { .name = "b", .minBound = 0, .maxBound = 1 };
    local_config.attr_domains[1] = attr_domain_b;

    struct cnode* cnode = make_cnode(&local_config, NULL);
    int id_gen = 1;
    const struct sub* sub1 = make_complex_sub(&id_gen, _AND(_EQ("a", 0), _EQ("b", 0)));
    const struct sub* sub2 = make_simple_sub(id_gen, "a", 0);
    id_gen++;
    const struct sub* sub3 = make_complex_sub(&id_gen, _AND(_EQ("a", 0), _EQ("b", 0)));
    const struct sub* sub4 = make_complex_sub(&id_gen, _AND(_EQ("a", 0), _EQ("b", 1)));

    insert_be_tree(&local_config, sub1, cnode, NULL);
    insert_be_tree(&local_config, sub2, cnode, NULL);
    insert_be_tree(&local_config, sub3, cnode, NULL);
    insert_be_tree(&local_config, sub4, cnode, NULL);

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


    mu_assert(
        lnode->sub_count == 0 &&
        pdir_a->pnode_count == 1 &&
        strcasecmp(pnode_a->attr, "a") == 0 &&
        strcasecmp(cdir_a->attr, "a") == 0 &&
        lnode_a->sub_count == 1 &&
        pdir_b->pnode_count == 1 &&
        strcasecmp(pnode_b->attr, "b") == 0 &&
        strcasecmp(cdir_b->attr, "b") == 0 && cdir_b->lChild == NULL && cdir_b->rChild == NULL &&
        lnode_b->sub_count == 3
    ,"tree matches what we expected");

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub");

    const struct event* event = make_simple_event("b", 1);

    struct matched_subs matched_subs;
    initialize_matched_subs(&matched_subs);
    match_be_tree(event, cnode, &matched_subs);
    mu_assert(matched_subs.sub_count == 1 && matched_subs.subs[0] == id_gen - 1, "goodEvent");

    free_event((struct event*)event);
    free_cnode((struct cnode*)cnode);
    return 0;
}

int all_tests() 
{
    config = malloc(sizeof(struct config));
    config->lnode_max_cap = 3;
    config->attr_domain_count = 3;
    config->attr_domains = malloc(3 * sizeof(struct attr_domain));
    struct attr_domain attr_domain_a = { .name = "a", .minBound = 0, .maxBound = 10 };
    config->attr_domains[0] = attr_domain_a;
    struct attr_domain attr_domain_b = { .name = "b", .minBound = 0, .maxBound = 10 };
    config->attr_domains[1] = attr_domain_b;
    struct attr_domain attr_domain_c = { .name = "c", .minBound = 0, .maxBound = 10 };
    config->attr_domains[2] = attr_domain_c;

    mu_run_test(test_sub_has_attribute);
    mu_run_test(test_remove_sub);
    mu_run_test(test_match_single_cnode);
    // mu_run_test(test_insert_first_split);
    // mu_run_test(test_pdir_split_twice);
    // mu_run_test(test_cdir_split_twice);
    // mu_run_test(test_remove_sub_in_tree);
    // mu_run_test(test_remove_sub_in_tree_with_delete);

    // mu_run_test(test_remove_sub_in_tree_with_two_levels_pnode);
    // mu_run_test(test_remove_sub_in_tree_with_two_levels_cdir);
    // mu_run_test(test_match_deeper);

    return 0;
}

RUN_TESTS()
