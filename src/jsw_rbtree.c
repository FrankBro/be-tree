#include "jsw_rbtree.h"

#include <stdlib.h>

#include "alloc.h"

#ifndef HEIGHT_LIMIT
#define HEIGHT_LIMIT 64 /* Tallest allowable tree */
#endif

struct jsw_rbnode {
    int red; // 1 = red, 0 = black
    void* data;
    struct jsw_rbnode* link[2]; // left, right
};

struct jsw_rbtree {
    struct jsw_rbnode* root;
    cmp_f cmp;
    size_t size;
};

static int is_red(struct jsw_rbnode* root)
{
    return root != NULL && root->red == 1;
}

static struct jsw_rbnode* jsw_single(struct jsw_rbnode* root, int dir)
{
    struct jsw_rbnode* save = root->link[!dir];

    root->link[!dir] = save->link[dir];
    save->link[dir] = root;

    root->red = 1;
    save->red = 0;

    return save;
}

static struct jsw_rbnode* jsw_double(struct jsw_rbnode* root, int dir)
{
    root->link[!dir] = jsw_single(root->link[!dir], !dir);

    return jsw_single(root, dir);
}

static struct jsw_rbnode* new_node(struct jsw_rbtree* tree, void* data)
{
    (void)tree;
    struct jsw_rbnode* rn = bmalloc(sizeof(*rn));

    if(rn == NULL) {
        return NULL;
	}

    rn->red = 1;
    rn->data = data;
    rn->link[0] = rn->link[1] = NULL;

    return rn;
}

struct jsw_rbtree* jsw_rbnew(cmp_f cmp)
{
    struct jsw_rbtree* rt = bmalloc(sizeof(*rt));

    if(rt == NULL) {
        return NULL;
	}

    rt->root = NULL;
    rt->cmp = cmp;
    rt->size = 0;

    return rt;
}

void jsw_rbdelete(struct jsw_rbtree* tree)
{
    struct jsw_rbnode* it = tree->root;
    struct jsw_rbnode* save;

    while(it != NULL) {
		if(it->link[0] == NULL) {
            save = it->link[1];
            bfree(it);
        }
        else {
            save = it->link[0];
            it->link[0] = save->link[1];
            save->link[1] = it;
        }

        it = save;
    }

    bfree(tree);
}

void* jsw_rbfind(struct jsw_rbtree* tree, void* data)
{
    struct jsw_rbnode* it = tree->root;

    while(it != NULL) {
        int cmp = tree->cmp(it->data, data);

        if(cmp == 0) {
            break;
		}

        it = it->link[cmp < 0];
    }

    return it == NULL ? NULL : it->data;
}

int jsw_rbinsert(struct jsw_rbtree* tree, void* data)
{
    if(tree->root == NULL) {
        tree->root = new_node(tree, data);

        if(tree->root == NULL) {
            return 0;
		}
    }
    else {
        struct jsw_rbnode head = {0}; 
        struct jsw_rbnode *g, *t;
        struct jsw_rbnode *p, *q;
        int dir = 0, last = 0;

        t = &head;
        g = p = NULL;
        q = t->link[1] = tree->root;

        for(;;) {
            if(q == NULL) {
                p->link[dir] = q = new_node(tree, data);

                if(q == NULL) {
                    return 0;
                }
            }
            else if(is_red(q->link[0]) && is_red(q->link[1])) {
                q->red = 1;
                q->link[0]->red = 0;
                q->link[1]->red = 0;
            }

            if(is_red(q) && is_red(p)) {
                int dir2 = t->link[1] == g;

                if(q == p->link[last]) {
                    t->link[dir2] = jsw_single(g, !last);
                }
                else {
                    t->link[dir2] = jsw_double(g, !last);
                }
            }

            if(tree->cmp(q->data, data) == 0) {
                break;
            }

            last = dir;
            dir = tree->cmp(q->data, data) < 0;

            if(g != NULL) {
                t = g;
            }

            g = p, p = q;
            q = q->link[dir];
        }

        tree->root = head.link[1];
    }

    tree->root->red = 0;
    tree->size++;

    return 1;
}

int jsw_rberase(struct jsw_rbtree* tree, void* data)
{
    if(tree->root != NULL) {
        struct jsw_rbnode head = {0}; /* False tree root */
        struct jsw_rbnode *q, *p, *g; /* Helpers */
        struct jsw_rbnode *f = NULL;  /* Found item */
        int dir = 1;

        q = &head;
        g = p = NULL;
        q->link[1] = tree->root;

        while(q->link[dir] != NULL) {
            int last = dir;

            g = p, p = q;
            q = q->link[dir];
            dir = tree->cmp(q->data, data) < 0;

            if(tree->cmp(q->data, data) == 0) {
                f = q;
            }

            if(!is_red(q) && !is_red(q->link[dir])) {
                if(is_red(q->link[!dir])) {
                    p = p->link[last] = jsw_single(q, dir);
                }
                else if(!is_red(q->link[!dir])) {
                    struct jsw_rbnode* s = p->link[!last];

                    if(s != NULL) {
                        if(!is_red(s->link[!last]) && !is_red(s->link[last])) {
                            p->red = 0;
                            s->red = 1;
                            q->red = 1;
                        }
                        else {
                            int dir2 = g->link[1] == p;

                            if(is_red(s->link[last])) {
                                g->link[dir2] = jsw_double(p, last);
                            }
                            else if(is_red(s->link[!last])) {
                                g->link[dir2] = jsw_single(p, last);
                            }

                            q->red = g->link[dir2]->red = 1;
                            g->link[dir2]->link[0]->red = 0;
                            g->link[dir2]->link[1]->red = 0;
                        }
                    }
                }
            }
        }

        if(f != NULL) {
            f->data = q->data;
            p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
            bfree(q);
        }

        tree->root = head.link[1];

        if(tree->root != NULL) {
            tree->root->red = 0;
        }

        tree->size--;
    }

    return 1;
}

size_t jsw_rbsize(struct jsw_rbtree* tree)
{
    return tree->size;
}

