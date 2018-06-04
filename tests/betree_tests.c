#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "debug.h"
#include "minunit.h"
#include "utils.h"

int parse(const char* text, struct ast_node** node);
int event_parse(const char* text, struct event** event);

struct sub* make_sub_from_expr(struct config* config, betree_sub_t id, const char* expr)
{
    struct ast_node* node;
    parse(expr, &node);
    struct sub* sub = make_sub(config, id, node);
    return sub;
}

int test_sub_has_attribute()
{
    struct config* config = make_default_config();
    struct sub* sub = make_sub_from_expr(config, 0, "a = 0");

    mu_assert(sub_has_attribute_str(config, sub, "a"), "Simple sub has 'a'");
    mu_assert(!sub_has_attribute_str(config, sub, "b"), "Simple sub does not have 'b'");

    free_sub(sub);
    free_config(config);
    return 0;
}

int test_remove_sub()
{
    struct config* config = make_default_config();
    struct cnode* cnode = make_cnode(config, NULL);

    betree_insert(config, 0, "a = 0", cnode);
    mu_assert(cnode->lnode->sub_count == 1 && cnode->lnode->subs[0]->id == 0, "lnode has one sub and it matches");

    betree_delete(config, 0, cnode);
    mu_assert(cnode->lnode->sub_count == 0, "lnode no longer has any subs");

    betree_insert(config, 0, "a = 0", cnode);
    betree_insert(config, 1, "a = 1", cnode);
    betree_insert(config, 2, "a = 2", cnode);
    mu_assert(cnode->lnode->sub_count == 3 && cnode->lnode->subs[0]->id == 0 && cnode->lnode->subs[1]->id == 1
            && cnode->lnode->subs[2]->id == 2,
        "lnode has three subs and they match");

    betree_delete(config, 1, cnode);
    mu_assert(cnode->lnode->sub_count == 2 && cnode->lnode->subs[0]->id == 0 && cnode->lnode->subs[1]->id == 2,
        "lnode no longer has sub2");

    free_cnode(cnode);
    free_config(config);
    return 0;
}

void reset_matched_subs(struct matched_subs* matched_subs)
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
    betree_insert(config, sub_id, "a = 0", cnode);

    mu_assert(cnode->lnode->sub_count == 1, "tree has one sub");


    struct matched_subs* matched_subs = make_matched_subs();
    {
        reset_matched_subs(matched_subs);
        betree_search(config, "{\"a\": 0}", cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 1 && matched_subs->subs[0] == sub_id, "goodEvent");
    }
    {
        reset_matched_subs(matched_subs);
        betree_search(config, "{\"a\": 1}", cnode, matched_subs, NULL);
        mu_assert(matched_subs->sub_count == 0, "wrongValueEvent");
    }
    {
        // TODO: When we support missing variables/non-missing
        // reset_matched_subs(&matched_subs);
        /*const struct event* wrongVariableEvent = make_event_from_string(config, "{\"b\": 0}");*/
        // betree_search(wrongVariableEvent, cnode, &matchedSub);
        // mu_assert(matched_subs.sub_count == 0, "wrongVariableEvent");
    }

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
    betree_insert(config, 0, "a = 0", cnode);
    betree_insert(config, 1, "a = 1", cnode);
    betree_insert(config, 2, "a = 2", cnode);
    betree_insert(config, 3, "b = 0", cnode);

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

    betree_insert(config, 1, "a = 0", cnode);
    betree_insert(config, 2, "a = 0", cnode);
    betree_insert(config, 3, "a = 0", cnode);

    mu_assert(cnode->lnode->sub_count == 3 &&
      cnode->lnode->subs[0]->id == 1 &&
      cnode->lnode->subs[1]->id == 2 &&
      cnode->lnode->subs[2]->id == 3
      , "subs123 in first lnode");

    betree_insert(config, 4, "b = 0", cnode);
    betree_insert(config, 5, "b = 0", cnode);
    betree_insert(config, 6, "b = 0", cnode);

    mu_assert(cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(cnode->lnode->sub_count == 3 &&
      cnode->lnode->subs[0]->id == 4 &&
      cnode->lnode->subs[1]->id == 5 &&
      cnode->lnode->subs[2]->id == 6
      , "subs456 in first lnode");

    betree_insert(config, 7, "c = 0", cnode);

    mu_assert(cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(cnode->pdir->pnodes[1]->cdir->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[0]->id == 4 &&
      cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[1]->id == 5 &&
      cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[2]->id == 6
      , "subs456 in second lnode");
    mu_assert(cnode->lnode->sub_count == 1 &&
      cnode->lnode->subs[0]->id == 7
      , "subs7 in first lnode");

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

    betree_insert(config, 1, "a = 2", cnode);
    betree_insert(config, 2, "a = 2", cnode);
    betree_insert(config, 3, "a = 2", cnode);

    mu_assert(cnode->lnode->sub_count == 3 &&
      cnode->lnode->subs[0]->id == 1 &&
      cnode->lnode->subs[1]->id == 2 &&
      cnode->lnode->subs[2]->id == 3
      , "subs123 in first lnode");

    betree_insert(config, 4, "b = 0", cnode);

    mu_assert(cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(cnode->lnode->sub_count == 1 &&
      cnode->lnode->subs[0]->id == 4
      , "subs4 in first lnode");

    betree_insert(config, 5, "a = 7", cnode);
    betree_insert(config, 6, "a = 7", cnode);
    betree_insert(config, 7, "a = 7", cnode);

    mu_assert(cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[0]->id == 1 &&
      cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[1]->id == 2 &&
      cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 3 &&
      cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[0]->id == 5 &&
      cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[1]->id == 6 &&
      cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[2]->id == 7
      , "subs567 in second lnode");
    mu_assert(cnode->lnode->sub_count == 1 &&
      cnode->lnode->subs[0]->id == 4
      , "subs4 in first lnode");

    free_cnode((struct cnode*)cnode);
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree()
{
    struct config* config = make_default_config();
    struct cnode* cnode = make_cnode(config, NULL);

    betree_insert(config, 0, "a = 0", cnode);

    mu_assert(cnode->lnode->sub_count == 1, "lnode has the sub");

    betree_delete(config, 0, cnode);

    mu_assert(cnode->lnode->sub_count == 0, "lnode does not have the sub");
    mu_assert(cnode != NULL && cnode->lnode != NULL,
        "did not delete the cnode or lnode because it's root");

    free_cnode(cnode);
    free_config(config);
    return 0;
}

int test_remove_sub_in_tree_with_delete()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, false);
    add_attr_domain_i(config, "b", 0, 10, false);
    struct cnode* cnode = make_cnode(config, NULL);

    betree_insert(config, 1, "a = 0", cnode);
    betree_insert(config, 2, "a = 0", cnode);
    betree_insert(config, 3, "a = 0", cnode);
    betree_insert(config, 4, "b = 0", cnode);

    mu_assert(cnode->lnode->sub_count == 1, "sub 4 is in lnode");
    mu_assert(cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "sub 1, 2, and 3 is lower lnode");

    betree_delete(config, 1, cnode);
    betree_delete(config, 2, cnode);
    betree_delete(config, 3, cnode);

    mu_assert(cnode->pdir == NULL, "deleted everything down of the pdir");

    free_cnode(cnode);
    free_config(config);
    return 0;
}

extern bool MATCH_NODE_DEBUG;

int test_match_deeper()
{
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 0, false);
    add_attr_domain_i(config, "b", 0, 1, false);

    struct cnode* cnode = make_cnode(config, NULL);

    betree_insert(config, 1, "a = 0 and b = 0", cnode);
    betree_insert(config, 2, "a = 1", cnode);
    betree_insert(config, 3, "a = 0 and b = 0", cnode);
    betree_insert(config, 4, "a = 0 and b = 1", cnode);

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

    struct matched_subs* matched_subs = make_matched_subs();
    reset_matched_subs(matched_subs);
    betree_search(config, "{\"a\": 0, \"b\": 1}", cnode, matched_subs, NULL);
    mu_assert(matched_subs->sub_count == 1 && matched_subs->subs[0] == 4, "goodEvent");

    free_cnode(cnode);
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
        char* expr;
        asprintf(&expr, "a = %zu", i);
        betree_insert(config, i, expr, cnode);
        free(expr);
    }

    struct matched_subs* matched_subs = make_matched_subs();
    betree_search(config, "{\"a\": 0}", cnode, matched_subs, NULL);

    mu_assert(matched_subs->sub_count == 1, "matched one");

    free_matched_subs(matched_subs);
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

    betree_insert(config, 0, "a = 0", cnode);
    betree_insert(config, 1, "a = 0", cnode);
    betree_insert(config, 2, "b = 0", cnode);
    betree_insert(config, 3, "c = 0", cnode);

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

    betree_insert(config, 0, "a = 0", cnode);
    betree_insert(config, 1, "a = 0", cnode);
    betree_insert(config, 2, "b = 0", cnode);
    betree_insert(config, 3, "c = 0", cnode);

    mu_assert(cnode->lnode->sub_count == 4, "First lnode has four subs");
    mu_assert(cnode->lnode->max != lnode_max_cap, "First lnode max cap went up");

    free_cnode(cnode);
    free_config(config);

    return 0;
}

int test_allow_undefined()
{
    enum { expr_count = 4 };
    const char* exprs[expr_count] = { "a = 0", "a = 1", "a = 0 or b = 0", "b = 1" };

    // With allow undefined
    struct config* config = make_default_config();
    add_attr_domain_i(config, "a", 0, 10, true);
    add_attr_domain_i(config, "b", 0, 10, false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < expr_count; i++) {
        const char* expr = exprs[i];
        betree_insert(config, i + 1, expr, cnode);
    }

    struct matched_subs* matched_subs = make_matched_subs();
    betree_search(config, "{\"b\": 0}", cnode, matched_subs, NULL);

    mu_assert(cnode->lnode->sub_count == 1 && cnode->pdir != NULL && cnode->pdir->pnode_count == 1
            && cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "Structure is what is expected");
    mu_assert(matched_subs->sub_count == 1, "Found the sub in the lower lnode");

    free_cnode(cnode);
    free_config(config);
    free_matched_subs(matched_subs);

    return 0;
}

int test_float()
{
    struct config* config = make_default_config();
    add_attr_domain_f(config, "a", 0., 10., false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? 0. : 7.;
        char* expr;
        asprintf(&expr, "a = %.1f", value);
        betree_insert(config, i, expr, cnode);
        free(expr);
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

    betree_insert(config, 0, "a = \"a\"", cnode);

    struct matched_subs* matched_subs = make_matched_subs();
    betree_search(config, "{\"a\": \"a\"}", cnode, matched_subs, NULL);

    mu_assert(matched_subs->sub_count == 1, "found our sub");

    free_cnode(cnode);
    free_config(config);
    free_matched_subs(matched_subs);

    return 0;
}

int test_string_wont_split()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "a", false);

    struct cnode* cnode = make_cnode(config, NULL);

    for(size_t i = 0; i < 4; i++) {
        betree_insert(config, i, "a = \"a\"", cnode);
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
        char* expr;
        asprintf(&expr, "a = %ld", value);
        betree_insert(config, i, expr, cnode);
        free(expr);
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
        double value = i < 3 ? -6. : -12.;
        char* expr;
        asprintf(&expr, "a = %.1f", value);
        betree_insert(config, i, expr, cnode);
        free(expr);
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

        betree_insert(config, 0, "a in (1, 2, 0)", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\": 0}", cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a not in (1, 2, 0)", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, "{\"a\": 0}", cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_integer_set_reverse()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "a", false);

    const char* event = "{\"a\": [1, 2, 0]}";

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "0 in a", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "0 not in a", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_string_set()
{
    struct config* config = make_default_config();
    add_attr_domain_s(config, "a", false);
    
    const char* event = "{\"a\": \"a\"}";

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a in (\"b\", \"c\", \"a\")", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a not in (\"b\", \"c\", \"a\")", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_string_set_reverse()
{
    struct config* config = make_default_config();
    add_attr_domain_sl(config, "a", false);

    const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "\"0\" in a", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "\"0\" not in a", cnode);

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "did not find our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    free_config(config);

    return 0;
}

int test_integer_list()
{
    struct config* config = make_default_config();
    add_attr_domain_il(config, "a", false);

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a one of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [1, 2, 0]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a one of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [4, 5, 3]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a none of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [4, 5, 3]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a none of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [1, 2, 0]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a all of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [1, 2, 0]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a all of (1, 2, 0)", cnode);

        const char* event = "{\"a\": [1, 2, 3]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
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

        betree_insert(config, 0, "a one of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a one of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a none of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a none of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a all of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 1, "found our sub");

        free_matched_subs(matched_subs);
        free_cnode(cnode);
    }

    {
        struct cnode* cnode = make_cnode(config, NULL);

        betree_insert(config, 0, "a all of (\"1\", \"2\", \"0\")", cnode);

        const char* event = "{\"a\": [\"1\", \"2\", \"3\"]}";

        struct matched_subs* matched_subs = make_matched_subs();
        betree_search(config, event, cnode, matched_subs, NULL);

        mu_assert(matched_subs->sub_count == 0, "found no sub");

        free_matched_subs(matched_subs);
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

int splitable_string_domain()
{
    struct config* config = make_default_config();
    add_attr_domain_bounded_s(config, "s", false, 5);

    struct cnode* cnode = make_cnode(config, NULL);
    betree_insert(config, 0, "s = \"0\"", cnode);
    betree_insert(config, 1, "s = \"1\"", cnode);
    betree_insert(config, 2, "s = \"2\"", cnode);
    betree_insert(config, 3, "s = \"3\"", cnode);
    betree_insert(config, 4, "s = \"4\"", cnode);

    mu_assert(!betree_can_insert(config, 5, "s = \"5\"", cnode), "can't insert another string value");

    mu_assert(cnode->lnode->sub_count == 0, "first lnode empty");
    mu_assert(cnode->pdir->pnode_count == 1, "has a pnode");
    mu_assert(cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
    mu_assert(cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3, "lchild has 3 subs");
    mu_assert(cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 2, "rchild has 2 subs");

    struct matched_subs* matched_subs = make_matched_subs();
    struct report report = make_empty_report();
    betree_search(config, "{\"s\": \"2\"}", cnode, matched_subs, &report);

    mu_assert(matched_subs->sub_count == 1, "matched 1");
    mu_assert(report.expressions_matched == 1 && report.expressions_evaluated == 3, "only had to evaluate lchild");

    free_config(config);
    free_cnode(cnode);
    free_matched_subs(matched_subs);
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
    mu_run_test(splitable_string_domain);

    return 0;
}

RUN_TESTS()
