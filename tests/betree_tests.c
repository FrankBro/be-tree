#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "debug.h"
#include "minunit.h"

const struct sub* make_simple_sub(struct config* config, unsigned int id, const char* attr, int value)
{
    struct sub* sub = make_empty_sub(id);
    sub->variable_id_count = 1;
    sub->variable_ids = malloc(sizeof(int));
    sub->variable_ids[0] = get_id_for_attr(config, attr);
    struct ast_node* expr = ast_binary_expr_create(AST_BINOP_EQ, attr, value);
    assign_variable_id(config, expr);
    sub->expr = expr;
    return sub;
}

const struct event* make_event_with_preds(const size_t size, const struct pred** preds)
{
    struct event* event = malloc(sizeof(struct event));
    event->pred_count = size;
    event->preds = malloc(sizeof(struct pred*) * size);
    memcpy(event->preds, preds, sizeof(struct pred*) * size);
    return event;
}

int test_sub_has_attribute() 
{
    struct config* config = make_default_config();
    const struct sub* sub = make_simple_sub(config, 0, "a", 0);

    mu_assert(sub_has_attribute_str(config, sub, "a"), "Simple sub has 'a'");
    mu_assert(!sub_has_attribute_str(config, sub, "b"), "Simple sub does not have 'b'");

    free_sub((struct sub*)sub);
    free_config(config);
    return 0;
}

int test_remove_sub()
{
    struct config* config = make_default_config();
    const struct sub* sub1 = make_simple_sub(config, 0, "a", 0);
    const struct sub* sub2 = make_simple_sub(config, 1, "a", 1);
    const struct sub* sub3 = make_simple_sub(config, 2, "a", 2);
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
    unsigned int subId = 0;
    struct sub* sub = (struct sub*)make_simple_sub(config, subId, "a", 0);
    insert_be_tree(config, sub, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub");

    const struct event* goodEvent = make_simple_event(config, "a", 0);
    const struct event* wrongValueEvent = make_simple_event(config, "a", 1);
    const struct event* wrongVariableEvent = make_simple_event(config, "b", 0);

    struct matched_subs* matched_subs = make_matched_subs();
    {
        initialize_matched_subs(matched_subs);
        match_be_tree(goodEvent, cnode, matched_subs);
        mu_assert(matched_subs->sub_count == 1 && matched_subs->subs[0] == subId, "goodEvent");
    }
    {
        initialize_matched_subs(matched_subs);
        match_be_tree(wrongValueEvent, cnode, matched_subs);
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
    unsigned int variable_id = get_id_for_attr(config, attr);
    return pnode->variable_id == variable_id;
}

bool test_attr_in_cdir(struct config* config, const char* attr, const struct cdir* cdir)
{
    if(config == NULL || cdir == NULL || attr == NULL) {
        return false;
    }
    unsigned int variable_id = get_id_for_attr(config, attr);
    return cdir->variable_id == variable_id;
}

int test_insert_first_split()
{
    struct config* config = make_default_config();
    add_attr_domain(config, "a", 0, 10);
    add_attr_domain(config, "b", 0, 10);
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(config, 0, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub(config, 1, "a", 1);
    struct sub* sub3 = (struct sub*)make_simple_sub(config, 2, "a", 2);
    struct sub* sub4 = (struct sub*)make_simple_sub(config, 3, "b", 0);
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
    mu_assert(pnode->cdir->lChild == NULL || pnode->cdir->rChild == NULL, "cdir has no lChild and rChild");
    mu_assert(pnode->cdir->cnode != NULL, "cdir has a cnode");
    mu_assert(pnode->cdir->cnode->pdir == NULL, "pdir in the lower cnode does not have a pdir");
    mu_assert(pnode->cdir->cnode->lnode != NULL, "inner cnode has a lnode");
    mu_assert(pnode->cdir->cnode->lnode->sub_count == 3, "tree has three sub in the second lnode");

    free_cnode((struct cnode*)cnode);
    free_config(config);
    return 0;
}

struct ast_node* _AND (const struct ast_node* lhs, const struct ast_node* rhs)
{
    return ast_combi_expr_create(AST_COMBI_AND, lhs, rhs);
}

struct ast_node* _GT (const char* attr, int value)
{
    return ast_binary_expr_create(AST_BINOP_GT, attr, value);
}

struct ast_node* _EQ (const char* attr, int value)
{
    return ast_binary_expr_create(AST_BINOP_EQ, attr, value);
}

struct ast_node* _LT (const char* attr, int value)
{
    return ast_binary_expr_create(AST_BINOP_LT, attr, value);
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

bool test_cnode_has_pnodes(struct config* config, const struct cnode* cnode, unsigned int pnode_count, const char** attrs)
{
    if(cnode->pdir == NULL || cnode->pdir->pnode_count != pnode_count) {
        return false;
    }
    for(unsigned int i = 0; i < pnode_count; i++) {
        if(!test_attr_in_pnode(config, attrs[i], cnode->pdir->pnodes[i])) {
            return false;
        }
    }
    return true;
}

int test_pdir_split_twice()
{
    struct config* config = make_default_config();
    add_attr_domain(config, "a", 0, 10);
    add_attr_domain(config, "b", 0, 10);
    add_attr_domain(config, "c", 0, 10);
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub(config, 1, "a", 0);
    const struct sub* sub2 = make_simple_sub(config, 2, "a", 0);
    const struct sub* sub3 = make_simple_sub(config, 3, "a", 0);
    const struct sub* sub4 = make_simple_sub(config, 4, "b", 0);
    const struct sub* sub5 = make_simple_sub(config, 5, "b", 0);
    const struct sub* sub6 = make_simple_sub(config, 6, "b", 0);
    const struct sub* sub7 = make_simple_sub(config, 7, "c", 0);

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
    free_config(config);
    return 0;
}

int test_cdir_split_twice()
{
    struct config* config = make_default_config();
    add_attr_domain(config, "a", 0, 10);
    struct cnode* cnode = make_cnode(config, NULL);

    const struct sub* sub1 = make_simple_sub(config, 1, "a", 2);
    const struct sub* sub2 = make_simple_sub(config, 2, "a", 2);
    const struct sub* sub3 = make_simple_sub(config, 3, "a", 2);
    const struct sub* sub4 = make_simple_sub(config, 4, "b", 0);
    const struct sub* sub5 = make_simple_sub(config, 5, "a", 7);
    const struct sub* sub6 = make_simple_sub(config, 6, "a", 7);
    const struct sub* sub7 = make_simple_sub(config, 7, "a", 7);

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
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree()
{
    struct config* config = make_default_config();
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(config, 0, "a", 0);

    insert_be_tree(config, sub1, cnode, NULL);

    mu_assert(cnode->lnode->sub_count == 1, "lnode has the sub");

    delete_be_tree(config, sub1, cnode);

    mu_assert(cnode->lnode->sub_count == 0, "lnode does not have the sub");
    mu_assert(cnode != NULL && cnode->lnode != NULL, "did not delete the cnode or lnode because it's root");

    free_cnode(cnode);
    free_sub(sub1);
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree_with_delete()
{
    struct config* config = make_default_config();
    add_attr_domain(config, "a", 0, 10);
    add_attr_domain(config, "b", 0, 10);
    struct cnode* cnode = make_cnode(config, NULL);
    struct sub* sub1 = (struct sub*)make_simple_sub(config, 1, "a", 0);
    struct sub* sub2 = (struct sub*)make_simple_sub(config, 2, "a", 0);
    struct sub* sub3 = (struct sub*)make_simple_sub(config, 3, "a", 0);
    struct sub* sub4 = (struct sub*)make_simple_sub(config, 4, "b", 0);

    insert_be_tree(config, sub1, cnode, NULL);
    insert_be_tree(config, sub2, cnode, NULL);
    insert_be_tree(config, sub3, cnode, NULL);
    insert_be_tree(config, sub4, cnode, NULL);

    const struct sub* subs123[3] = { sub1, sub2, sub3 };
    const struct sub* subs4[1] = { sub4 };
    mu_assert(test_lnode_has_subs(cnode->lnode, 1, subs4), "sub 4 is in lnode");
    mu_assert(test_lnode_has_subs(cnode->pdir->pnodes[0]->cdir->cnode->lnode, 3, subs123), "sub 1, 2, and 3 is lower lnode");

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
    add_attr_domain(config, "a", 0, 0);
    add_attr_domain(config, "b", 0, 1);

    struct cnode* cnode = make_cnode(config, NULL);
    const struct sub* sub1 = make_sub(config, 1, _AND(_EQ("a", 0), _EQ("b", 0)));
    const struct sub* sub2 = make_simple_sub(config, 2, "a", 1);
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


    mu_assert(
        lnode->sub_count == 0 &&
        pdir_a->pnode_count == 1 &&
        test_attr_in_pnode(config, "a", pnode_a) &&
        test_attr_in_cdir(config, "a", cdir_a) &&
        lnode_a->sub_count == 1 &&
        pdir_b->pnode_count == 1 &&
        test_attr_in_pnode(config, "b", pnode_b) &&
        test_attr_in_cdir(config, "b", cdir_b) &&
        cdir_b->lChild == NULL && cdir_b->rChild == NULL &&
        lnode_b->sub_count == 3
    , "tree matches what we expected");

    mu_assert(lnode_a->sub_count == 1, "lnode in 'a' has one sub");
    mu_assert(lnode_b->sub_count == 3, "lnode in 'a' has one sub");

    const struct pred* pred_a = make_simple_pred_str(config, "a", 0);
    const struct pred* pred_b = make_simple_pred_str(config, "b", 1);
    const struct pred* preds[2] = { pred_a, pred_b };
    const struct event* event = make_event_with_preds(2, preds);

    struct matched_subs* matched_subs = make_matched_subs();
    initialize_matched_subs(matched_subs);
    match_be_tree(event, cnode, matched_subs);
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
    add_attr_domain(config, "a", 0, 10000);

    struct cnode* cnode = make_cnode(config, NULL);
    
    for(unsigned int i = 0; i < 100; i++) {
        const struct sub* sub = make_simple_sub(config, i, "a", i);
        insert_be_tree(config, sub, cnode, NULL);
    }

    const struct event* event = make_simple_event(config, "a", 0);
    struct matched_subs* matched_subs = make_matched_subs();
    match_be_tree(event, cnode, matched_subs);

    mu_assert(matched_subs->sub_count == 1, "matched one");

    free_matched_subs(matched_subs);
    free_event((struct event*)event);
    free_cnode(cnode);
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

    return 0;
}

RUN_TESTS()
