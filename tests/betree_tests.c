#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "betree.h"
#include "debug.h"
#include "helper.h"
#include "minunit.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

int test_sub_has_attribute()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");

    struct betree_sub* sub = tree->cnode->lnode->subs[0];
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
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 1
        && cnode->lnode->subs[0]->id == sub1;
}

bool cnode_has_sub2(struct cnode* cnode, betree_sub_t sub1, betree_sub_t sub2)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 2
        && cnode->lnode->subs[0]->id == sub1 && cnode->lnode->subs[1]->id == sub2;
}
bool cnode_has_sub3(struct cnode* cnode, betree_sub_t sub1, betree_sub_t sub2, betree_sub_t sub3)
{
    return cnode != NULL && cnode->lnode != NULL && cnode->lnode->sub_count == 3
        && cnode->lnode->subs[0]->id == sub1 && cnode->lnode->subs[1]->id == sub2
        && cnode->lnode->subs[2]->id == sub3;
}

bool report_has_sub0(struct report* report)
{
    return report != NULL && report->matched == 0;
}

bool report_has_sub1(struct report* report, betree_sub_t sub1)
{
    return report != NULL && report->matched == 1 && report->subs[0] == sub1;
}

int test_match_single_cnode()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);

    betree_sub_t sub_id = 0;

    mu_assert(betree_insert(tree, sub_id, "a = 0"), "");
    mu_assert(cnode_has_sub1(tree->cnode, sub_id), "tree has one sub");

    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\": 0}", report), "");
        mu_assert(report_has_sub1(report, sub_id), "good event");
        free_report(report);
    }
    {
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\": 1}", report), "");
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
    betree_var_t variable_id = try_get_id_for_attr(config, attr);
    return pnode->attr_var.var == variable_id;
}

bool cdir_has_attr(struct config* config, const char* attr, const struct cdir* cdir)
{
    if(config == NULL || cdir == NULL || attr == NULL) {
        return false;
    }
    betree_var_t variable_id = try_get_id_for_attr(config, attr);
    return cdir->attr_var.var == variable_id;
}

int test_insert_first_split()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);

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

    // write_dot_file(tree);
    // write_dot_to_file(tree, "/tmp/betree.dot");

    betree_free(tree);
    return 0;
}

bool test_cnode_has_pnodes(
    struct config* config, const struct cnode* cnode, size_t pnode_count, const char** attrs)
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
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "c", false, 0, 10);

    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "a = 0"), "");
    mu_assert(betree_insert(tree, 3, "a = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 3 && tree->cnode->lnode->subs[0]->id == 1
            && tree->cnode->lnode->subs[1]->id == 2 && tree->cnode->lnode->subs[2]->id == 3,
        "subs123 in first lnode");

    mu_assert(betree_insert(tree, 4, "b = 0"), "");
    mu_assert(betree_insert(tree, 5, "b = 0"), "");
    mu_assert(betree_insert(tree, 6, "b = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3,
        "subs123 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 3 && tree->cnode->lnode->subs[0]->id == 4
            && tree->cnode->lnode->subs[1]->id == 5 && tree->cnode->lnode->subs[2]->id == 6,
        "subs456 in first lnode");

    mu_assert(betree_insert(tree, 7, "c = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3,
        "subs123 in second lnode");
    mu_assert(tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[0]->id == 4
            && tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[1]->id == 5
            && tree->cnode->pdir->pnodes[1]->cdir->cnode->lnode->subs[2]->id == 6,
        "subs456 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 && tree->cnode->lnode->subs[0]->id == 7,
        "subs7 in first lnode");

    betree_free(tree);
    return 0;
}

int test_cdir_split_twice()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);

    mu_assert(betree_insert(tree, 1, "a = 2"), "");
    mu_assert(betree_insert(tree, 2, "a = 2"), "");
    mu_assert(betree_insert(tree, 3, "a = 2"), "");

    mu_assert(tree->cnode->lnode->sub_count == 3 && tree->cnode->lnode->subs[0]->id == 1
            && tree->cnode->lnode->subs[1]->id == 2 && tree->cnode->lnode->subs[2]->id == 3,
        "subs123 in first lnode");

    mu_assert(betree_insert(tree, 4, "b = 0"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[0]->id == 1
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[1]->id == 2
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->subs[2]->id == 3,
        "subs123 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 && tree->cnode->lnode->subs[0]->id == 4,
        "subs4 in first lnode");

    mu_assert(betree_insert(tree, 5, "a = 7"), "");
    mu_assert(betree_insert(tree, 6, "a = 7"), "");
    mu_assert(betree_insert(tree, 7, "a = 7"), "");

    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[0]->id == 1
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[1]->id == 2
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->subs[2]->id == 3,
        "subs123 in second lnode");
    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 3
            && tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[0]->id == 5
            && tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[1]->id == 6
            && tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->subs[2]->id == 7,
        "subs567 in second lnode");
    mu_assert(tree->cnode->lnode->sub_count == 1 && tree->cnode->lnode->subs[0]->id == 4,
        "subs4 in first lnode");

    betree_free(tree);
    return 0;
}

/*int test_remove_sub_in_tree()*/
/*{*/
    /*struct betree* tree = betree_make();*/
    /*add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);*/

    /*mu_assert(betree_insert(tree, 0, "a = 0"), "");*/

    /*mu_assert(tree->cnode->lnode->sub_count == 1, "lnode has the sub");*/

    /*mu_assert(betree_delete(tree, 0), "");*/

    /*mu_assert(tree->cnode->lnode->sub_count == 0, "lnode does not have the sub");*/
    /*mu_assert(tree->cnode != NULL && tree->cnode->lnode != NULL,*/
        /*"did not delete the cnode or lnode because it's root");*/

    /*betree_free(tree);*/
    /*return 0;*/
/*}*/

/*int test_remove_sub_in_tree_with_delete()*/
/*{*/
    /*struct betree* tree = betree_make();*/
    /*add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);*/
    /*add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);*/

    /*mu_assert(betree_insert(tree, 1, "a = 0"), "");*/
    /*mu_assert(betree_insert(tree, 2, "a = 0"), "");*/
    /*mu_assert(betree_insert(tree, 3, "a = 0"), "");*/
    /*mu_assert(betree_insert(tree, 4, "b = 0"), "");*/

    /*mu_assert(tree->cnode->lnode->sub_count == 1, "sub 4 is in lnode");*/
    /*mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 3,*/
        /*"sub 1, 2, and 3 is lower lnode");*/

    /*mu_assert(betree_delete(tree, 1), "");*/
    /*mu_assert(betree_delete(tree, 2), "");*/
    /*mu_assert(betree_delete(tree, 3), "");*/

    /*mu_assert(tree->cnode->pdir == NULL, "deleted everything down of the pdir");*/

    /*betree_free(tree);*/
    /*return 0;*/
/*}*/

int test_match_deeper()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 0);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 1);

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
            && pnode_has_attr(tree->config, "a", pnode_a)
            && cdir_has_attr(tree->config, "a", cdir_a) && lnode_a->sub_count == 1
            && pdir_b->pnode_count == 1 && pnode_has_attr(tree->config, "b", pnode_b)
            && cdir_has_attr(tree->config, "b", cdir_b) && cdir_b->lchild == NULL
            && cdir_b->rchild == NULL && lnode_b->sub_count == 3,
        "tree matches what we expected");

    mu_assert(lnode_a->sub_count == 1, "lnode in 'a' has one sub");
    mu_assert(lnode_b->sub_count == 3, "lnode in 'a' has one sub");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"a\": 0, \"b\": 1}", report), "");
    mu_assert(report->matched == 1 && report->subs[0] == 4, "goodEvent");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_large_cdir_split()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10000);

    for(size_t i = 0; i < 100; i++) {
        char* expr;
        if(basprintf(&expr, "a = %zu", i) < 0) {
            abort();
        }
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"a\": 0}", report), "");

    mu_assert(report->matched == 1, "matched one");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_min_partition()
{
    size_t lnode_max_cap = 3;
    // With 0
    struct betree* tree = betree_make_with_parameters(lnode_max_cap, 0);
    ;
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "c", false, 0, 10);

    mu_assert(betree_insert(tree, 0, "a = 0"), "");
    mu_assert(betree_insert(tree, 1, "a = 0"), "");
    mu_assert(betree_insert(tree, 2, "b = 0"), "");
    mu_assert(betree_insert(tree, 3, "c = 0"), "");

    mu_assert(tree->cnode->lnode->sub_count == 2, "First lnode has two subs");
    mu_assert(tree->cnode->pdir->pnode_count == 1 && tree->cnode->pdir->pnodes[0]->attr_var.var == 0
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 2,
        "Has a pnode for 'a' and two subs");

    betree_free(tree);

    // With 3
    tree = betree_make_with_parameters(lnode_max_cap, 3);
    ;
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);
    add_attr_domain_bounded_i(tree->config, "c", false, 0, 10);

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
    add_attr_domain_bounded_i(tree->config, "a", true, 0, 10);
    add_attr_domain_bounded_i(tree->config, "b", false, 0, 10);

    for(size_t i = 0; i < expr_count; i++) {
        const char* expr = exprs[i];
        mu_assert(betree_insert(tree, i + 1, expr), "");
    }

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"b\": 0}", report), "");

    mu_assert(tree->cnode->lnode->sub_count == 2 && tree->cnode->pdir != NULL
            && tree->cnode->pdir->pnode_count == 1
            && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 2,
        "Structure is what is expected");
    mu_assert(report->matched == 1, "Found the sub in the lower lnode");

    betree_free(tree);
    free_report(report);

    return 0;
}

int test_float()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_f(tree->config, "a", false, 0., 10.);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? 0. : 7.;
        char* expr;
        if(basprintf(&expr, "a = %.1f", value) < 0) {
            abort();
        }
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct pnode* pnode = tree->cnode->pdir->pnodes[0];
    mu_assert(tree->cnode->pdir != NULL && tree->cnode->pdir->pnode_count == 1
            && pnode->cdir != NULL && pnode->cdir->cnode->lnode->sub_count == 0
            && pnode->cdir->lchild != NULL && pnode->cdir->rchild != NULL
            && pnode->cdir->lchild->cnode->lnode->sub_count == 3
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
    mu_assert(betree_search(tree, "{\"a\": \"a\"}", report), "");

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
    add_attr_domain_bounded_i(tree->config, "a", false, min, max);

    for(size_t i = 0; i < 4; i++) {
        int64_t value = i < 3 ? -6 : -12;
        char* expr;
        if(basprintf(&expr, "a = %ld", value) < 0) {
            abort();
        }
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == BETREE_INTEGER && cdir->bound.imin == min
            && cdir->bound.imax == max && lchild->bound.value_type == BETREE_INTEGER
            && lchild->bound.imin == min && lchild->bound.imax == mid
            && rchild->bound.value_type == BETREE_INTEGER && rchild->bound.imin == mid
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
    add_attr_domain_bounded_f(tree->config, "a", false, min, max);

    for(size_t i = 0; i < 4; i++) {
        double value = i < 3 ? -6. : -12.;
        char* expr;
        if(basprintf(&expr, "a = %.1f", value) < 0) {
            abort();
        }
        mu_assert(betree_insert(tree, i, expr), "");
        free(expr);
    }

    const struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
    const struct cdir* lchild = cdir->lchild;
    const struct cdir* rchild = cdir->rchild;

    mu_assert(cdir->bound.value_type == BETREE_FLOAT && feq(cdir->bound.fmin, min)
            && feq(cdir->bound.fmax, max) && lchild->bound.value_type == BETREE_FLOAT
            && feq(lchild->bound.fmin, min) && feq(lchild->bound.fmax, mid)
            && rchild->bound.value_type == BETREE_FLOAT && feq(rchild->bound.fmin, mid)
            && feq(rchild->bound.fmax, max),
        "cdirs have proper bounds");

    betree_free(tree);

    return 0;
}

int test_integer_set()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "a", false, 0, 10);

    {
        mu_assert(betree_insert(tree, 0, "a in (1, 2, 0)"), "");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\": 0}", report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a not in (1, 2, 0)"), "");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\": 0}", report), "");

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
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "0 not in a"), "");

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

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
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a not in (\"b\", \"c\", \"a\")"), "");

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

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
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "\"0\" not in a"), "");

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "did not find our sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

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
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a one of (1, 2, 0)"), "");

        const char* event = "{\"a\": [4, 5, 3]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (1, 2, 0)"), "");

        const char* event = "{\"a\": [4, 5, 3]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 0]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 0]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (1, 2, 0)"), "");

        const char* event = "{\"a\": [1, 2, 3]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

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
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a one of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"4\", \"5\", \"3\"]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a none of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"0\"]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 1, "found our sub");

        free_report(report);
        empty_tree(tree);
    }

    {
        mu_assert(betree_insert(tree, 0, "a all of (\"1\", \"2\", \"0\")"), "");

        const char* event = "{\"a\": [\"1\", \"2\", \"3\"]}";

        struct report* report = make_report();
        mu_assert(betree_search(tree, event, report), "");

        mu_assert(report->matched == 0, "found no sub");

        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

    return 0;
}

int test_parenthesis()
{
    struct betree* tree = betree_make();
    add_attr_domain_b(tree->config, "a", false);
    add_attr_domain_b(tree->config, "b", false);
    add_attr_domain_b(tree->config, "c", false);

    {
        mu_assert(betree_insert(tree, 1, "a || (b && c)"), "");
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\":true,\"b\":false,\"c\":false}", report), "");
        mu_assert(report->matched == 1, "");
        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "(a || b) && c"), "");
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\":true,\"b\":false,\"c\":false}", report), "");
        mu_assert(report->matched == 0, "");
        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "a || (b && c)"), "");
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"a\":false,\"b\":true,\"c\":true}", report), "");
        mu_assert(report->matched == 1, "");
        empty_tree(tree);
        free_report(report);
    }

    betree_free(tree);

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
        mu_assert(
            tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3,
            "lchild has 3 subs");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 2,
            "rchild has 2 subs");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"s\": \"1\"}", report), "");

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
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 1,
            "one sub in the top cdir");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild != NULL
                && tree->cnode->pdir->pnodes[0]->cdir->rchild != NULL
                && tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 1,
            "cdir split as expected");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"s\": \"2\"}", report), "");

        mu_assert(report->matched == 1, "matched 1");
        mu_assert(report->evaluated == 2, "will eval 2 because 0 is in first cdir, second cdir is open_right and therefore will eval");

        betree_free(tree);
        free_report(report);
    }
    return 0;
}

int test_not_domain_changing()
{
    struct betree* tree = betree_make();
    tree->config->lnode_max_cap = 1;
    add_attr_domain_bounded_i(tree->config, "i", false, 0, 10);

    {
        mu_assert(betree_insert(tree, 1, "not (i > 2)"), "");
        mu_assert(betree_insert(tree, 2, "not (i < 7)"), "");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1
                && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0,
            "second lnode empty");
        struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
        mu_assert(cdir->lchild != NULL && cdir->rchild != NULL
                && cdir->lchild->cnode->lnode->sub_count == 1
                && cdir->rchild->cnode->lnode->sub_count == 1,
            "lchild and rchild contain our subs");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 1}", report), "");

        mu_assert(report->matched == 1 && report->subs[0] == 1 && report->evaluated == 1,
            "only evaluated and found the correct expressions");

        empty_tree(tree);
        free_report(report);
    }
    {
        mu_assert(betree_insert(tree, 1, "not (not (i < 2))"), "");
        mu_assert(betree_insert(tree, 2, "not (not (i > 7))"), "");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1
                && tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0,
            "second lnode empty");
        struct cdir* cdir = tree->cnode->pdir->pnodes[0]->cdir;
        mu_assert(cdir->lchild != NULL && cdir->rchild != NULL
                && cdir->lchild->cnode->lnode->sub_count == 1
                && cdir->rchild->cnode->lnode->sub_count == 1,
            "lchild and rchild contain our subs");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 1}", report), "");

        mu_assert(report->matched == 1 && report->subs[0] == 1 && report->evaluated == 1,
            "only evaluated and found the correct expressions");

        empty_tree(tree);
        free_report(report);
    }

    betree_free(tree);

    return 0;
}

/*
int test_insert_all()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "i1", true, 0, 10);
    add_attr_domain_bounded_i(tree->config, "i2", true, 0, 10);

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
    mu_assert(betree_search(tree, "{\"i1\": 0}", report), "");
    mu_assert(report->matched == 3, "correct match");

    free_report(report);
    betree_free(tree);
    return 0;
}
*/

int test_bug_cases()
{
    struct betree* tree = betree_make_with_parameters(1, 0);
    add_attr_domain_b(tree->config, "b", false);
    add_attr_domain_bounded_i(tree->config, "i", true, 0, 10);

    mu_assert(betree_insert(tree, 0, "not b || i > 8"), "");
    mu_assert(betree_insert(tree, 1, "not b"), "");
    mu_assert(betree_insert(tree, 2, "b && i < 8"), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"b\": true, \"i\": 9}", report), "");
    mu_assert(report->matched == 1, "correct match");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_splitable_integer_list_domain()
{
    {
        struct betree* tree = betree_make();
        add_attr_domain_bounded_il(tree->config, "il", false, 0, 10);

        mu_assert(betree_insert(tree, 0, "1 in il"), "");
        mu_assert(betree_insert(tree, 1, "2 in il"), "");
        mu_assert(betree_insert(tree, 2, "3 in il"), "");
        mu_assert(betree_insert(tree, 3, "6 in il"), "");
        mu_assert(betree_insert(tree, 4, "7 in il"), "");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1, "has a pnode");
        mu_assert(
            tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3,
            "lchild has 3 subs");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 2,
            "rchild has 2 subs");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"il\": [2]}", report), "");

        mu_assert(report->matched == 1, "matched 1");
        mu_assert(report->evaluated == 3, "only had to evaluate lchild");

        betree_free(tree);
        free_report(report);
    }

    return 0;
}

int test_splitable_string_list_domain()
{
    {
        struct betree* tree = betree_make();
        add_attr_domain_bounded_sl(tree->config, "sl", false, 5);

        mu_assert(betree_insert(tree, 0, "\"0\" in sl"), "");
        mu_assert(betree_insert(tree, 1, "\"1\" in sl"), "");
        mu_assert(betree_insert(tree, 2, "\"2\" in sl"), "");
        mu_assert(betree_insert(tree, 3, "\"3\" in sl"), "");
        mu_assert(betree_insert(tree, 4, "\"4\" in sl"), "");

        mu_assert(!betree_insert(tree, 5, "\"5\" in sl"), "can't insert another string value");

        mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
        mu_assert(tree->cnode->pdir->pnode_count == 1, "has a pnode");
        mu_assert(
            tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 3,
            "lchild has 3 subs");
        mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 2,
            "rchild has 2 subs");

        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"sl\": [\"1\"]}", report), "");

        mu_assert(report->matched == 1, "matched 1");
        mu_assert(report->evaluated == 3, "only had to evaluate lchild");

        betree_free(tree);
        free_report(report);
    }

    return 0;
}

int test_set_bug_cdir()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_s(tree->config, "s", false, 4);

    mu_assert(betree_insert(tree, 0, "s = \"a\""), "");
    mu_assert(betree_insert(tree, 1, "s = \"b\""), "");
    mu_assert(betree_insert(tree, 2, "s = \"c\""), "");
    mu_assert(betree_insert(tree, 3, "s = \"d\""), "");
    mu_assert(betree_insert(tree, 4, "s in (\"b\", \"c\", \"d\")"), "");

    mu_assert(tree->cnode->lnode->sub_count == 0, "first lnode empty");
    mu_assert(tree->cnode->pdir->pnode_count == 1, "has a pnode");
    mu_assert(
        tree->cnode->pdir->pnodes[0]->cdir->cnode->lnode->sub_count == 0, "second lnode empty");
    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->lnode->sub_count == 2,
        "lchild has 2 subs");
    mu_assert(tree->cnode->pdir->pnodes[0]->cdir->rchild->cnode->lnode->sub_count == 3,
        "rchild has 3 subs");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"s\": \"c\"}", report), "");

    mu_assert(report->matched == 2, "matched 2");
    mu_assert(report->evaluated == 3, "only had to evaluate lchild");

    betree_free(tree);
    free_report(report);
    return 0;
}

int test_undefined_cdir_search()
{
    struct betree* tree = betree_make_with_parameters(1, 1);
    add_attr_domain_b(tree->config, "b", true);
    add_attr_domain_bounded_i(tree->config, "i", true, 0, 10);

    const char* event = "{\"i\": 0}";

    mu_assert(betree_insert(tree, 0, "not b && i = 0"), "");

    struct report* report;
    report = make_report();

    mu_assert(betree_search(tree, event, report), "");

    mu_assert(
        report->evaluated == 1 && report->matched == 1, "true because false on undef, then not");

    free_report(report);
    report = make_report();

    mu_assert(betree_insert(tree, 1, "not b && i = 10"), "");

    mu_assert(tree->cnode->lnode->sub_count == 0 && tree->cnode->pdir != NULL
            && tree->cnode->pdir->pnode_count == 1
            && strcmp(tree->cnode->pdir->pnodes[0]->attr_var.attr, "b") == 0
            && tree->cnode->pdir->pnodes[0]->cdir != NULL
            && tree->cnode->pdir->pnodes[0]->cdir->lchild != NULL
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->pdir != NULL
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->pdir->pnode_count == 1
            && strcmp(tree->cnode->pdir->pnodes[0]
                          ->cdir->lchild->cnode->pdir->pnodes[0]
                          ->attr_var.attr,
                   "i")
                == 0
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->pdir->pnodes[0]->cdir != NULL
            && tree->cnode->pdir->pnodes[0]->cdir->lchild->cnode->pdir->pnodes[0]->cdir->lchild
                != NULL
            && tree->cnode->pdir->pnodes[0]
                    ->cdir->lchild->cnode->pdir->pnodes[0]
                    ->cdir->lchild->cnode->pdir
                == NULL
            && tree->cnode->pdir->pnodes[0]
                    ->cdir->lchild->cnode->pdir->pnodes[0]
                    ->cdir->lchild->cnode->lnode->sub_count
                == 1,
        "expected structure, split on b, all went in lchild for only false, first then i and split "
        "into lchild/rchild");

    mu_assert(betree_search(tree, event, report), "");

    mu_assert(report->evaluated == 1 && report->matched == 1, "still true");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_api() 
{
    struct betree* tree = betree_make();
    betree_add_boolean_variable(tree, "b", false);
    betree_add_integer_variable(tree, "i", false, INT64_MIN, INT64_MAX);
    betree_add_float_variable(tree, "f", false, -DBL_MAX, DBL_MAX);
    betree_add_string_variable(tree, "s", false, SIZE_MAX);
    betree_add_integer_list_variable(tree, "il", false, INT64_MIN, INT64_MAX);
    betree_add_string_list_variable(tree, "sl", false, SIZE_MAX);
    betree_add_segments_variable(tree, "seg", false);
    betree_add_frequency_caps_variable(tree, "frequency_caps", false);
    betree_add_integer_variable(tree, "now", false, INT64_MIN, INT64_MAX);

    const char* expr =
             "b and "
             "i = 10 and "
             "f > 3.13 and "
             "s = \"good\" and "
             "1 in il and "
             "sl none of (\"good\") and "
             "segment_within(seg, 1, 20) and "
             "within_frequency_cap(\"flight\", \"ns\", 100, 0)";
    enum e { constant_count = 4 };
    const struct betree_constant* constants[constant_count] = {
        betree_make_integer_constant("flight_id", 10),
        betree_make_integer_constant("advertiser_id", 20),
        betree_make_integer_constant("campaign_id", 30),
        betree_make_integer_constant("product_id", 40),
    };
    mu_assert(betree_insert_with_constants(tree, 0, constant_count, constants, expr), "");
    for(size_t i = 0; i < constant_count; i++) {
        betree_free_constant((struct betree_constant*)constants[i]);
    }

    struct betree_event* event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_boolean_variable("b", true));
    betree_set_variable(event, 1, betree_make_integer_variable("i", 10));
    betree_set_variable(event, 2, betree_make_float_variable("f", 3.14));
    betree_set_variable(event, 3, betree_make_string_variable("s", "good"));
    struct betree_integer_list* il = betree_make_integer_list(3);
    betree_add_integer(il, 0, 1);
    betree_add_integer(il, 1, 2);
    betree_add_integer(il, 2, 3);
    betree_set_variable(event, 4, betree_make_integer_list_variable("il", il));
    struct betree_string_list* sl = betree_make_string_list(1);
    betree_add_string(sl, 0, "bad");
    betree_set_variable(event, 5, betree_make_string_list_variable("sl", sl));
    struct betree_segments* seg = betree_make_segments(1);
    int64_t usec = 1000 * 1000;
    betree_add_segment(seg, 0, betree_make_segment(1, 10 * usec));
    betree_set_variable(event, 6, betree_make_segments_variable("seg", seg));
    struct betree_frequency_caps* frequency_caps = betree_make_frequency_caps(1);
    betree_add_frequency_cap(frequency_caps, 0, betree_make_frequency_cap("flight", 10, "ns", false, 0, 0));
    betree_set_variable(event, 7, betree_make_frequency_caps_variable("frequency_caps", frequency_caps));
    betree_set_variable(event, 8, betree_make_integer_variable("now", 0));

    struct report* report = make_report();
    mu_assert(betree_search_with_event(tree, event, report), "");

    mu_assert(report->matched == 1, "found 1");

    betree_free_event(event);
    betree_free(tree);
    free_report(report);

    return 0;
}

int test_inverted_binop()
{
    struct betree* tree = betree_make();
    betree_add_integer_variable(tree, "i", false, 0, 30);

    {
        mu_assert(betree_insert(tree, 1, "20 < i"), "");
        mu_assert(betree_insert(tree, 2, "i > 20"), "");
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 20}", report), "");
        mu_assert(report->matched == 0, "no match");
        free_report(report);
        empty_tree(tree);
    }
    {
        mu_assert(betree_insert(tree, 1, "20 < i"), "");
        mu_assert(betree_insert(tree, 2, "i > 20"), "");
        struct report* report = make_report();
        mu_assert(betree_search(tree, "{\"i\": 21}", report), "");
        mu_assert(report->matched == 2, "all match");
        free_report(report);
        empty_tree(tree);
    }

    betree_free(tree);

    return 0;
}

int test_float_no_point_in_expr()
{
    struct betree* tree = betree_make();
    betree_add_float_variable(tree, "f", false, 0, 20.);

    mu_assert(betree_insert(tree, 1, "f = 20"), "");

    struct betree_event* event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_float_variable("f", 20.));

    struct report* report = make_report();

    mu_assert(betree_search_with_event(tree, event, report), "");

    mu_assert(report->matched == 1, "found 1");

    betree_free_event(event);
    betree_free(tree);
    free_report(report);


    return 0;
}

int test_is_null()
{
    struct betree* tree = betree_make();
    betree_add_integer_variable(tree, "def", false, 0, 10);
    betree_add_integer_variable(tree, "undef", true, 0, 10);

    mu_assert(betree_insert(tree, 1, "undef is not null"), "");

    struct betree_event* event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_integer_variable("def", 5));

    struct report* report = make_report();

    mu_assert(betree_search_with_event(tree, event, report), "");

    mu_assert(report->matched == 0, "found none");

    betree_free_event(event);
    betree_free(tree);
    free_report(report);

    return 0;
}

int test_bug_geo()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "exchange", false);
    add_attr_domain_i(tree->config, "member_id", false);
    add_attr_domain_f(tree->config, "latitude", true);
    add_attr_domain_f(tree->config, "longitude", true);
    add_attr_domain_i(tree->config, "width", false);
    add_attr_domain_i(tree->config, "height", false);
    add_attr_domain_il(tree->config, "types", false);

    mu_assert(betree_insert(tree, 0, "(((width is not null and width = 100) and (height is not null and height = 200) "
        "and (types is not null and 1 in types) and true and true)) and (((exchange is not null and exchange = 2) "
        "and (member_id is not null and member_id = 0) and true))"), "");
    mu_assert(betree_insert(tree, 1, "(((width is not null and width = 100) and (height is not null and height = 200) "
        "and (types is not null and 1 in types) and true and true)) and (((exchange is not null and exchange = 2) "
        "and (member_id is not null and member_id = 0) and true)) and geo_within_radius(100.0, 100.0, 10.0)"), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"latitude\": 100.0, \"longitude\": 100.0, \"exchange\": 2, \"member_id\": 0, \"width\": 100, \"height\": 200, \"types\": [1]}", report), "");

    mu_assert(report->matched == 2, "");

    betree_free(tree);
    free_report(report);
    return 0;
}

int test_list_bug1()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_il(tree->config, "il", false, 1, 2);

    for(size_t i = 0; i < 5; i++) {
        mu_assert(betree_insert(tree, i, "1 in il"), "");
    }

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"il\": [1,2]}", report), "");

    mu_assert(report->evaluated != 0, "");
    mu_assert(report->matched == 5, "");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_list_bug2()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_il(tree->config, "il", false, 1, 5);

    for(size_t i = 0; i < 5; i++) {
        mu_assert(betree_insert(tree, i, "il one of (1)"), "");
    }

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"il\": [1,2,3,4,5]}", report), "");

    mu_assert(report->evaluated != 0, "");
    mu_assert(report->matched == 5, "");

    free_report(report);
    betree_free(tree);
    return 0;
}

int test_event_out_of_bound()
{
    struct betree* tree = betree_make();
    add_attr_domain_bounded_i(tree->config, "i", false, 0, 10);

    mu_assert(betree_insert(tree, 0, "i > 9"), "");
    mu_assert(betree_insert(tree, 1, "i > 9"), "");
    mu_assert(betree_insert(tree, 2, "i > 9"), "");
    mu_assert(betree_insert(tree, 3, "i > 9"), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"i\": 15}", report), "");

    mu_assert(report->matched == 4, "");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_int_enum()
{
    struct betree* tree = betree_make();
    add_attr_domain_ie(tree->config, "i", false);

    const char* expr1 = "i = 7822930";
    const char* expr2 = "i = 9";
    const char* expr3 = "i = 86428";
    const char* expr4 = "i = 984981931";
    const char* expr5 = "i = 828";

    mu_assert(betree_change_boundaries(tree, expr1), "");
    mu_assert(betree_change_boundaries(tree, expr2), "");
    mu_assert(betree_change_boundaries(tree, expr3), "");
    mu_assert(betree_change_boundaries(tree, expr4), "");
    mu_assert(betree_change_boundaries(tree, expr5), "");

    mu_assert(tree->config->attr_domains[0]->bound.value_type == BETREE_INTEGER_ENUM
      && tree->config->attr_domains[0]->bound.smax == 4, "Managed to influence the boundaries");

    mu_assert(betree_insert(tree, 1, expr1), "");
    mu_assert(betree_insert(tree, 2, expr2), "");
    mu_assert(betree_insert(tree, 3, expr3), "");
    mu_assert(betree_insert(tree, 4, expr4), "");
    mu_assert(betree_insert(tree, 5, expr5), "");

    mu_assert(tree->cnode->pdir != NULL, "Managed to split");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"i\": 9}", report), "");

    mu_assert(report->matched == 1, "Found our item");
    mu_assert(report->evaluated == 3, "Did not evaluate everything");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_same_id()
{
    struct betree* tree = betree_make();
    add_attr_domain_i(tree->config, "i", false);

    mu_assert(betree_insert(tree, 1,  "i = 0"), "");
    mu_assert(betree_insert(tree, 1,  "i = 0"), "");
    mu_assert(betree_insert(tree, 1,  "i = 1"), "");

    struct report* report = make_report();
    mu_assert(betree_search(tree, "{\"i\": 0}", report), "");

    mu_assert(report->matched == 2, "Found the same");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_frequency_bug()
{
    struct betree* tree = betree_make();
    add_attr_domain_frequency(tree->config, "frequency_caps", false);
    add_attr_domain_i(tree->config, "now", false);

    const char* expr = "within_frequency_cap(\"flight:ip\", \"3495614\", 1, 5184000)";

    const struct betree_constant* constants[3] = {
        betree_make_integer_constant("campaign_id", 50650),
        betree_make_integer_constant("advertiser_id", 6573),
        betree_make_integer_constant("flight_id", 101801),
    };
    const struct betree_sub* sub = betree_make_sub(tree, 0, 3, constants, expr);
    betree_insert_sub(tree, sub);
    for(size_t i = 0; i < 3; i++) {
        betree_free_constant((struct betree_constant*)constants[i]);
    }

    const char* event = "{\"now\": 1541704800, \"frequency_caps\": [[[\"flight:ip\",101801,\"3495614\"],1,1546537569676283]]}";
    struct report* report = make_report();
    mu_assert(betree_search(tree, event, report), "");

    mu_assert(report->matched == 0, "");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_duplicate_unsorted_integer_list()
{
    struct betree* tree = betree_make();
    add_attr_domain_il(tree->config, "il", false);

    const char* expr = "3 in il";

    const struct betree_sub* sub = betree_make_sub(tree, 0, 0, NULL, expr);
    betree_insert_sub(tree, sub);

    const char* event = "{\"il\": [2,3,1,4,3,2,1,3]}";
    struct report* report = make_report();
    mu_assert(betree_search(tree, event, report), "");

    mu_assert(report->matched == 1, "");

    free_report(report);
    betree_free(tree);

    return 0;
}

int test_duplicate_unsorted_string_list()
{
    struct betree* tree = betree_make();
    add_attr_domain_sl(tree->config, "sl", false);

    const char* expr = "\"3\" in sl";

    const struct betree_sub* sub = betree_make_sub(tree, 0, 0, NULL, expr);
    betree_insert_sub(tree, sub);

    const char* event = "{\"sl\": [\"2\",\"3\",\"1\",\"4\",\"3\",\"2\",\"1\",\"3\"]}";
    struct report* report = make_report();
    mu_assert(betree_search(tree, event, report), "");

    mu_assert(report->matched == 1, "");

    free_report(report);
    betree_free(tree);

    return 0;
}

int all_tests()
{
    mu_run_test(test_int_enum);
    mu_run_test(test_sub_has_attribute);
    mu_run_test(test_match_single_cnode);
    mu_run_test(test_insert_first_split);
    mu_run_test(test_pdir_split_twice);
    mu_run_test(test_cdir_split_twice);
    /*mu_run_test(test_remove_sub_in_tree);*/
    /*mu_run_test(test_remove_sub_in_tree_with_delete);*/
    mu_run_test(test_match_deeper);
    mu_run_test(test_large_cdir_split);
    mu_run_test(test_min_partition);
    mu_run_test(test_allow_undefined);
    mu_run_test(test_float);
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
    mu_run_test(test_bug_cases);
    mu_run_test(test_splitable_integer_list_domain);
    mu_run_test(test_splitable_string_list_domain);
    mu_run_test(test_set_bug_cdir);
    mu_run_test(test_undefined_cdir_search);
    mu_run_test(test_api);
    mu_run_test(test_inverted_binop);
    mu_run_test(test_float_no_point_in_expr);
    mu_run_test(test_is_null);
    mu_run_test(test_bug_geo);
    mu_run_test(test_event_out_of_bound);
    mu_run_test(test_list_bug1);
    mu_run_test(test_list_bug2);
    mu_run_test(test_same_id);
    mu_run_test(test_frequency_bug);
    mu_run_test(test_duplicate_unsorted_integer_list);
    mu_run_test(test_duplicate_unsorted_string_list);

    return 0;
}

RUN_TESTS()

