#include <stdio.h>

#include "betree.h"

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

void print_cnode(const struct config* config, const struct cnode* cnode, unsigned int level);

void print_cdir(const struct config* config, const struct cdir* cdir, unsigned int level)
{
    if(cdir == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    printf(" cdir [%d, %d]", cdir->startBound, cdir->endBound);
    if(cdir->cnode != NULL) {
        printf("\n");
        print_cnode(config, cdir->cnode, level + 1);
    }
    if(cdir->lChild != NULL) {
        printf("\n");
        print_cdir(config, cdir->lChild, level + 1);
    }
    if(cdir->rChild != NULL) {
        printf("\n");
        print_cdir(config, cdir->rChild, level + 1);
    }
}

void print_pnode(const struct config* config, const struct pnode* pnode, unsigned int level)
{
    if(pnode == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    printf(" pnode %s (%f)", attr, pnode->score);
    if(pnode->cdir != NULL) {
        printf("\n");
        print_cdir(config, pnode->cdir, level + 1);
    }
}

void print_pdir(const struct config* config, const struct pdir* pdir, unsigned int level)
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
            print_pnode(config, pnode, level + 1);
        }
    }
}

void print_cnode(const struct config* config, const struct cnode* cnode, unsigned int level)
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
        print_pdir(config, cnode->pdir, level + 1);
    }
}

void print_be_tree(const struct config* config, const struct cnode* root)
{
    print_cnode(config, root, 0);
}
