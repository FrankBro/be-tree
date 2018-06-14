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

int test_sub_has_attribute()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    
    mu_assert(betree_insert(tree, 0, "a = 0"), "");

    struct sub* sub = tree->cnode->lnode->subs[0];
    mu_assert(sub_has_attribute_str(tree->config, sub, "a"), "Simple sub has 'a'");
    mu_assert(!sub_has_attribute_str(tree->config, sub, "b"), "Simple sub does not have 'b'");

    betree_free(tree);
    return 0;
}

bool cnode_has_sub0(struct cnode* cnode)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 0;
}
bool cnode_has_sub1(struct cnode* cnode, betree_sub_t sub1)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 1 && cnode->lnode->subs[0]->id == sub1;
}

bool cnode_has_sub2(struct cnode* cnode, betree_sub_t sub1, betree_sub_t sub2)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 2 && cnode->lnode->subs[0]->id == sub1 && cnode->lnode->subs[1]->id == sub2;
}
bool cnode_has_sub3(struct cnode* cnode, betree_sub_t sub1, betree_sub_t sub2, betree_sub_t sub3)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 3 && cnode->lnode->subs[0]->id == sub1 && cnode->lnode->subs[1]->id == sub2 && cnode->lnode->subs[2]->id == sub3;
}

bool report_has_sub0(struct report* report)
{
    return report != NULL && report->matched == 0;
}

bool report_has_sub1(struct report* report, betree_sub_t sub1)
{
    return report != NULL && report->matched == 1 && report->subs[0] == sub1;
}

int test_remove_sub()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(cnode_has_sub1(tree->cnode, 0), "lnode has one sub and it matches");

    mu_assert(betree_delete(tree, 0), "");
    mu_assert(cnode_has_sub0(tree->cnode), "lnode no longer has any subs");

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(betree_insert(tree, 1, "a = 1"), "");
    mu_assert(betree_insert(tree, 2, "a = 2"), "");
    mu_assert(cnode_has_sub3(tree->cnode, 0, 1, 2), "lnode has three subs and they match");

    mu_assert(betree_delete(tree, 1), "");
    mu_assert(cnode_has_sub2(tree->cnode, 0, 2), "lnode no longer has sub2");

    betree_free(tree);
    return 0;
}

int test_match_single_cnode()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);

    betree_sub_t sub_id = 0;

    mu_assert(betree_insert(tree, sub_id, "a = 0"), "");
    mu_assert(cnode_has_sub1(tree->cnode, sub_id), "tree has one sub");

    {
        struct report* report = make_report();
        betree_search(tree, "{\"a\": 0}", report);
        mu_assert(report_has_sub1(report, sub_id), "good event");
        free_report(report);
    }
    {
        struct report* report = make_report();
        betree_search(tree, "{\"a\": 1}", report);
        mu_assert(report_has_sub0(report), "bad event");
        free_report(report);
    }

    betree_free(tree);
    return 0;
}

bool pnode_has_attr(struct config* config, const char* attr, const struct pnode* pnode)
{
    if(config == NULL || pnode == NULL || attr == NULL) {
        return false;
    }
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return pnode->attr_var.var == variable_id;
}

bool cdir_has_attr(struct config* config, const char* attr, const struct cdir* cdir)
{
    if(config == NULL || cdir == NULL || attr == NULL) {
        return false;
    }
    betree_var_t variable_id = get_id_for_attr(config, attr);
    return cdir->attr_var.var == variable_id;
}

int test_insert_first_split()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(betree_insert(tree, 1, "a = 1"), "");
    mu_assert(betree_insert(tree, 2, "a = 2"), "");
    mu_assert(betree_insert(tree, 3, "b = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 1, "tree has one sub in the first lnode");
    mu_assert(tree->cnode->pdir != NULL, "cnode has a pdir");
    mu_assert(tree->cnode->pdir->pnode_count == 1, "cnode has a pdir with one pnode");
    struct pnode* pnode = tree->cnode->pdir->pnodes[0];
    mu_assert(pnode->cdir != NULL, "pnode has a cdir");
    mu_assert(pnode_has_attr(tree->config, "a", pnode), "pnode is for attr 'a'");
    mu_assert(cdir_has_attr(tree->config, "a", pnode->cdir), "cdir is for attr 'a'");
    mu_assert(pnode->cdir->lchild == NULL || pnode->cdir->rchild == NULL,
        "cdir has no lChild and rChild");
    mu_assert(pnode->cdir->cnode != NULL, "cdir has a cnode");
    mu_assert(pnode->cdir->cnode->pdir == NULL, "pdir in the lower cnode does not have a pdir");
    mu_assert(pnode->cdir->cnode->lnode != NULL, "inner cnode has a lnode");
    mu_assert(pnode->cdir->cnode->lnode->sub_count == 3, "tree has three sub in the second lnode");

    betree_free(tree);
    return 0;
}

bool test_cnode_has_pnodes(struct config* config, const struct cnode* cnode, size_t pnode_count, const char** attrs)
{
    if(cnode->pdir == NULL || cnode->pdir->pnode_count != pnode_count) {
        return false;
    }
    for(size_t i = 0; i < pnode_count; i++) {
        if(!pnode_has_attr(config, attrs[i], cnode->pdir->pnodes[i])) {
            return false;
        }
    }
    return true;
}

int test_pdir_split_twice()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);
    add_attr_domain_i(tree->config, "c", 0, 10, false);

    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 0"), "");
    mu_assert(betree_insert(tree, 3, "a = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 3 &&
      tree->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->lnode->subs[2]->id == 3
      , "subs123 in first lnode");

    mu_assert(betree_insert(tree, 4, "b = 0"), "");
    mu_assert(betree_insert(tree, 5, "b = 0"), "");
    mu_assert(betree_insert(tree, 6, "b = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 3 &&
      tree->cnode->lnode->subs[0]->id == 4 &&
      tree->cnode->lnode->subs[1]->id == 5 &&
      tree->cnode->lnode->subs[2]->id == 6
      , "subs456 in first lnode");

    mu_assert(betree_insert(tree, 7, "c = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[0]->id == 4 &&
      tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[1]->id == 5 &&
      tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[2]->id == 6
      , "subs456 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 &&
      tree->cnode->lnode->subs[0]->id == 7
      , "subs7 in first lnode");

    betree_free(tree);
    return 0;
}

int test_cdir_split_twice()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);

    mu_assert(betree_insert(tree, 1, "a = 2"), "");
    mu_assert(betree_insert(tree, 2, "a = 2"), "");
    mu_assert(betree_insert(tree, 3, "a = 2"), "");

    mu_assert(tree->cnode->lnode->sub_count == 3 &&
      tree->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->lnode->subs[2]->id == 3
      , "subs123 in first lnode");

    mu_assert(betree_insert(tree, 4, "b = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 &&
      tree->cnode->lnode->subs[0]->id == 4
      , "subs4 in first lnode");

    mu_assert(betree_insert(tree, 5, "a = 7"), "");
    mu_assert(betree_insert(tree, 6, "a = 7"), "");
    mu_assert(betree_insert(tree, 7, "a = 7"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[0]->id == 1 &&
      tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[1]->id == 2 &&
      tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[2]->id == 3
      , "subs123 in second lnode");
    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 3 &&
      tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[0]->id == 5 &&
      tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[1]->id == 6 &&
      tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[2]->id == 7
      , "subs567 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 &&
      tree->cnode->lnode->subs[0]->id == 4
      , "subs4 in first lnode");

    betree_free(tree);
    return 0;
}

int test_remove_sub_in_tree()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 1, "lnode has the sub");

    mu_assert(betree_delete(tree, 0), "");

    mu_assert(tree->cnode->lnode->sub_count == 0, "lnode does not have the sub");
    mu_assert(tree->cnode != NULL && tree->cnode->lnode != NULL,
        "did not delete the cnode or lnode because it's root");

    betree_free(tree);
    return 0;
}

int test_remove_sub_in_tree_with_delete()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);

    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 0"), "");
    mu_assert(betree_insert(tree, 3, "a = 0"), "");
    mu_assert(betree_insert(tree, 4, "b = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 1, "sub 4 is in lnode");
    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "sub 1, 2, and 3 is lower lnode");

    mu_assert(betree_delete(tree, 1), "");
    mu_assert(betree_delete(tree, 2), "");
    mu_assert(betree_delete(tree, 3), "");

    mu_assert(tree->cnode->pdir == NULL, "deleted everything down of the pdir");

    betree_free(tree);
    return 0;
}

extern bool MATCH_NODE_DEBUG;

int test_match_deeper()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 0, false);
    add_attr_domain_i(tree->config, "b", 0, 1, false);

    mu_assert(betree_insert(tree, 1, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 1"), "");
    mu_assert(betree_insert(tree, 3, "a = 0 and b = 0"), "");
    mu_assert(betree_insert(tree, 4, "a = 0 and b = 1"), "");

    const struct lnode* lnode = tree->cnode->lnode;
    const struct pdir* pdir_a = tree->cnode->pdir;
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
            && pnode_has_attr(tree->config, "a", pnode_a) && cdir_has_attr(tree->config, "a", cdir_a)
            && lnode_a->sub_count == 1 && pdir_b->pnode_count == 1
            && pnode_has_attr(tree->config, "b", pnode_b) && cdir_has_attr(tree->config, "b", cdir_b)
            && cdir_b->lchild == NULL && cdir_b->rchild == NULL && lnode_b->sub_count == 3,
        "tree matches what we expected");

    mu_assert(lnode_a->sub_count == 1, "lnode in 'a' has one sub");
    mu_assert(lnode_b->sub_count == 3, "lnode in 'a' has one sub");

    struct report* report = make_report();
    betree_search(tree, "{\"a\": 0, \"b\": 1}", report);
    mu_assert(report->matched == 1 && report->subs[0] == 4, "goodEvent");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_large_cdir_split()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10000, false);

    for(size_t i = 0; i < 100; i++) {
        char* expr;
        asprintf(&expr, "a = %zu", i);
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    struct report* report = make_report();
    betree_search(tree, "{\"a\": 0}", report);

    mu_assert(report->matched == 1, "matched one");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_min_partition()
{
    size_t lnode_max_cap = 3;
    // With 0
    struct betree* tree = betree_make_with_parameters(lnode_max_cap, 0);;
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);
    add_attr_domain_i(tree->config, "c", 0, 10, false);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "b = 0"), "");
    mu_assert(betree_insert(tree, 3, "c = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 2, "First lnode has two subs");
    mu_assert(tree->cnode->pdir->pnode_count == 1 && 
      tree->cnode->pdir->pnodes[0]->attr_var.var == 0 && 
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 2,
        "Has a pnode for 'a' and two subs");

    betree_free(tree);

    // With 3
    tree = betree_make_with_parameters(lnode_max_cap, 3);;
    add_attr_domain_i(tree->config, "a", 0, 10, false);
    add_attr_domain_i(tree->config, "b", 0, 10, false);
    add_attr_domain_i(tree->config, "c", 0, 10, false);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "b = 0"), "");
    mu_assert(betree_insert(tree, 3, "c = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 4, "First lnode has four subs");
    mu_assert(tree->cnode->lnode->max != lnode_max_cap, "First lnode max cap went up");

    betree_free(tree);

    return 0;
}

int test_allow_undefined()
{
    enum { expr_count = 4 };
    const char* exprs[expr_count] = { "a = 0", "a = 1", "a = 0 or b = 0", "b = 1" };

    // With allow undefined
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, true);
    add_attr_domain_i(tree->config, "b", 0, 10, false);

    for(size_t i = 0; i < expr_count; i++) {
        const char* expr = exprs[i];
        mu_assert(betree_insert(tree, i + 1, expr), "");
    }

    struct report* report = make_report();
    betree_search(tree, "{\"b\": 0}", report);

    mu_assert(tree->cnode->lnode->sub_count == 1 && tree->cnode->pdir != NULL && tree->cnode->pdir->pnode_count == 1
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,
        "Structure is what is expected");
    mu_assert(report->matched == 1, "Found the sub in the lower lnode");

    betree_free(tree);
    free_report(report);

    return 0;
}

int test_float()
{
    struct betree* tree = betree_make();
    add_attr_domain_f(tree->config, "a", 0., 10., false);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? 0. : 7.;
        char* expr;
        asprintf(&expr, "a = %.1f", value);
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct pnode* pnode = tree->cnode->pdir->pnodes[0];
    mu_assert(tree->cnode->pdir != NULL && tree->cnode->pdir->pnode_count == 1 && pnode->cdir != NULL
            && pnode->cdir->cnode->lnode->sub_count == 0 && pnode->cdir->lchild != NULL
            && pnode->cdir->rchild != NULL && pnode->cdir->lchild->cnode->lnode->sub_count == 3
            && pnode->cdir->rchild->cnode->lnode->sub_count == 1,
        "structure is respected");

    betree_free(tree);

    return 0;
}

// int test_bool()
// {
//     struct betree* tree = betree_make();
//     add_attr_domain_b(tree->config, "a", false, true, false);

//     struct cnode* cnode = make_cnode(tree->config, NULL);

//     for(size_t i = 0; i < 4; i++) {
//         enum ast_bool_e op = i < 3 ? AST_BOOL_NOT : AST_BOOL_NONE;
//         struct sub* sub = (struct sub*)make_simple_sub_b(tree->config, i, "a", op);
//         insert_be_tree(tree->config, sub, cnode, NULL);
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

//     return 0;
// }

int test_string()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", false);

    mu_assert(betree_insert(tree, 0, "a = \"a\""), "");

    struct report* report = make_report();
    betree_search(tree, "{\"a\": \"a\"}", report);

    mu_assert(report->matched == 1, "found our sub");

    betree_free(tree);
    free_report(report);

    return 0;
}

int test_string_wont_split()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", false);

    for(size_t i = 0; i < 4; i++) {
        mu_assert(betree_insert(tree, i, "a = \"a\""), "");
    }

    mu_assert(tree->cnode->lnode->sub_count == 4, "did not split");

    betree_free(tree);

    return 0;
}

int test_negative_int()
{
    struct betree* tree = betree_make();
    int64_t min = -10;
    int64_t max = -4;
    int64_t mid = (min + max) / 2;
    add_attr_domain_i(tree->config, "a", min, max, false);

    for(size_t i = 0; i < 4; i++) {
        int64_t value = i < 3 ? -6 : -12;
        char* expr;
        asprintf(&expr, "a = %ld", value);
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == VALUE_I && cdir->bound.imin == min
            && cdir->bound.imax == max && lchild->bound.value_type == VALUE_I
            && lchild->bound.imin == min && lchild->bound.imax == mid
            && rchild->bound.value_type == VALUE_I && rchild->bound.imin == mid
            && rchild->bound.imax == max,
        "cdirs have proper bounds");

    betree_free(tree);

    return 0;
}

int test_negative_float()
{
    struct betree* tree = betree_make();
    double min = -10.;
    double max = -4.;
    double mid = ceil((min + max) / 2.);
    add_attr_domain_f(tree->config, "a", min, max, false);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? -6. : -12.;
        char* expr;
        asprintf(&expr, "a = %.1f", value);
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == VALUE_F && feq(cdir->bound.fmin, min)
            && feq(cdir->bound.fmax, max) && lchild->bound.value_type == VALUE_F
            && feq(lchild->bound.fmin, min) && feq(lchild->bound.fmax, mid)
            && rchild->bound.value_type == VALUE_F && feq(rchild->bound.fmin, mid)
            && feq(rchild->bound.fmax, max),
        "cdirs have proper bounds");

    betree_free(tree);

    return 0;
}

void empty_tree(struct betree* tree)
{
    if(tree->cnode != NULL) {
        free_cnode(tree->cnode);
        tree->cnode = make_cnode(tree->config, NULL);
    }
}

int test_integer_set()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "a", 0, 10, false);

    {
        mu_assert(betree_insert(tree, 0, "a in (1, 2, 0)"), "");

        struct report* report = make_report();
        betree_search(tree, "{\"a\": 0}", report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a not in (1, 2, 0)"), "");

        struct report* report = make_report();
        betree_search(tree, "{\"a\": 0}", report);

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

    return 0;
}

int test_integer_set_reverse()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "a", false);

    const char* event = "{\"a\": [1, 2, 0]}";

    {
        mu_assert(betree_insert(tree, 0, "0 in a"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "0 not in a"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

    return 0;
}

int test_string_set()
{
    struct betree* tree = betree_make();
    add_attr_domain_s(tree->config, "a", false);
    
    const char* event = "{\"a\": \"a\"}";

    {
        mu_assert(betree_insert(tree, 0, "a in (\"b\", \"c\", \"a\")"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a not in (\"b\", \"c\", \"a\")"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    return 0;
}

int test_string_set_reverse()
{
    struct betree* tree = betree_make();
    add_attr_domain_sl(tree->config, "a", false);

    const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

    {
        mu_assert(betree_insert(tree, 0, "\"0\" in a"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "\"0\" not in a"), "");

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    return 0;
}

int test_integer_list()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "a", false);

    {
        mu_assert(betree_insert(tree, 0, "a one of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 0]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a one of (1, 2, 0)"), "");

        const char* event = "{\"a\": [4, 5, 3]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (1, 2, 0)"), "");

        const char* event = "{\"a\": [4, 5, 3]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 0]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 0]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 3]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    return 0;
}

int test_string_list()
{
    struct betree* tree = betree_make();
    add_attr_domain_sl(tree->config, "a", false);

    {
        mu_assert(betree_insert(tree, 0, "a one of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a one of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"3\"]}";

        struct report* report = make_report();
        betree_search(tree, event, report);

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    return 0;
}

int event_parse(const char* text, struct event** event);

int test_parenthesis()
{
    struct betree* tree = betree_make();
    add_attr_domain_b(tree->config, "a", false, true, false);
    add_attr_domain_b(tree->config, "b", false, true, false);
    add_attr_domain_b(tree->config, "c", false, true, false);

    {
        mu_assert(betree_insert(tree, 1, "a || (b && c)"), "");
        struct report* report = make_report();
        betree_search(tree, "{\"a\":true,\"b\":false,\"c\":false}", report);
        mu_assert(report->matched == 1, "");
        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "(a || b) && c"), "");
        struct report* report = make_report();
        betree_search(tree, "{\"a\":true,\"b\":false,\"c\":false}", report);
        mu_assert(report->matched == 0, "");
        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "a || (b && c)"), "");
        struct report* report = make_report();
        betree_search(tree, "{\"a\":false,\"b\":true,\"c\":true}", report);
        mu_assert(report->matched == 1, "");
        empty_tree(tree);
        free_report(report);
    }

    return 0;
}

int test_splitable_string_domain()
{
    // Simple test
    {
        struct betree* tree = betree_make();
        add_attr_domain_bounded_s(tree->config, "s", false, 5);

        mu_assert(betree_insert(tree, 0, "s = \"0\""), "");
        mu_assert(betree_insert(tree, 1, "s = \"1\""), "");
        mu_assert(betree_insert(tree, 2, "s = \"2\""), "");
        mu_assert(betree_insert(tree, 3, "s = \"3\""), "");
        mu_assert(betree_insert(tree, 4, "s = \"4\""), "");

        mu_assert(!betree_insert(tree, 5, "s = \"5\""), "can't insert another string value");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1, "has a pnode");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3, "lchild has 3 subs");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 2, "rchild has 2 subs");

        struct report* report = make_report();
        betree_search(tree, "{\"s\": \"2\"}", report);

        mu_assert(report->matched == 1, "matched 1");
        mu_assert(report->evaluated == 3, "only had to evaluate lchild");

        betree_free(tree);
        free_report(report);
    }

    // Make sure we can find <> tests for strings that don't fit in bound
    {
        struct betree* tree = betree_make();
        tree->config->lnode_max_cap = 1;
        add_attr_domain_bounded_s(tree->config, "s", false, 2);

        mu_assert(betree_insert(tree, 0, "s <> \"0\""), "");
        mu_assert(betree_insert(tree, 1, "s = \"1\""), "");

        mu_assert(!betree_insert(tree, 2, "s = \"2\""), "can't insert another string value");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1, "a pdir was created");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 1, "one sub in the top cdir");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild != NULL && 
          tree->cnode->pdir->pnodes[0]->cdir->rchild != NULL &&
          tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 1, "cdir split as expected");

        struct report* report = make_report();
        betree_search(tree, "{\"s\": \"2\"}", report);

        mu_assert(report->matched == 1, "matched 1");
        mu_assert(report->evaluated == 1, "only had to evaluate top");

        betree_free(tree);
        free_report(report);
    }
    return 0;
}

int test_not_domain_changing()
{
    struct betree* tree = betree_make();
    tree->config->lnode_max_cap = 1;
    add_attr_domain_i(tree->config, "i", 0, 10, false);

    {
        mu_assert(betree_insert(tree, 1, "not (i > 2)"), "");
        mu_assert(betree_insert(tree, 2, "not (i < 7)"), "");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1 &&
          tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
        mu_assert(cdir->lchild != NULL && cdir->rchild != NULL &&
          cdir->lchild->cnode->lnode->sub_count == 1 &&
          cdir->rchild->cnode->lnode->sub_count == 1, "lchild and rchild contain our subs");

        struct report* report = make_report();
        betree_search(tree, "{\"i\": 1}", report);

        mu_assert(report->matched == 1 && 
          report->subs[0] == 1 &&
          report->evaluated == 1 
          , "only evaluated and found the correct expressions");

        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "not (not (i < 2))"), "");
        mu_assert(betree_insert(tree, 2, "not (not (i > 7))"), "");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1 &&
          tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
        mu_assert(cdir->lchild != NULL && cdir->rchild != NULL &&
          cdir->lchild->cnode->lnode->sub_count == 1 &&
          cdir->rchild->cnode->lnode->sub_count == 1, "lchild and rchild contain our subs");

        struct report* report = make_report();
        betree_search(tree, "{\"i\": 1}", report);

        mu_assert(report->matched == 1 && 
          report->subs[0] == 1 &&
          report->evaluated == 1
          , "only evaluated and found the correct expressions");

        empty_tree(tree);
        free_report(report);
    }

    return 0;
}

int test_insert_all()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i1", 0, 10, true);
    add_attr_domain_i(tree->config, "i2", 0, 10, true);

    const char* exprs[3] = {
        "i1 = 0 || i1 = 2",
        "i1 = 0 || i1 = 4",
        "i1 = 0 || i1 = 6"
    };
    betree_insert_all(tree, 3, exprs);
    mu_assert(tree->cnode->lnode->sub_count == 3, "did not split yet");

    mu_assert(betree_insert(tree, 5, "i2 = 0 || i2 = 2"), "");
    mu_assert(tree->cnode->lnode->sub_count == 1 &&
      tree->cnode->pdir != NULL &&
      tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3, "split");

    struct report* report = make_report();
    betree_search(tree, "{\"i1\": 0}", report);
    mu_assert(report->matched == 3, "correct match");

    free_report(report);
    betree_free(tree);
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
    mu_run_test(test_splitable_string_domain);
    mu_run_test(test_not_domain_changing);
    mu_run_test(test_insert_all);

    return 0;
}

RUN_TESTS()
