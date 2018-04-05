#include <stdio.h>
#include <string.h>

#include "betree.h"

#define SEP_DASH  "------------------------------------------------------"
#define SEP_SPACE "                                                      "

void print_lnode(const struct lnode* lnode, unsigned int level)
{
    if(lnode == NULL) {
        return;
    }
    printf("%.*s", level * 2, SEP_DASH);
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
    printf("%.*s", level * 2, SEP_DASH);
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
    printf("%.*s", level * 2, SEP_DASH);
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
    printf("%.*s", level * 2, SEP_DASH);
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
    printf("%.*s", level * 2, SEP_DASH);
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

// -----------------------------------------------------------------------------
// dot
// -----------------------------------------------------------------------------

const char* get_path_cnode(const struct config* config, const struct cnode* cnode);

const char* get_path_pnode(const struct config* config, const struct pnode* pnode)
{
    char* name;
    const char* parent_path = get_path_cnode(config, pnode->parent->parent);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    asprintf(&name, "%s_%s", parent_path, attr);
    free((char*)parent_path);
    return name;
}

const char* get_path_cdir(const struct config* config, const struct cdir* cdir, bool first)
{
    switch(cdir->parent_type) {
        case CNODE_PARENT_CDIR: {
            if(first) {
                char* name;
                const char* parent_path = get_path_cdir(config, cdir->cdir_parent, false);
                asprintf(&name, "%s_%d_%d", parent_path, cdir->startBound, cdir->endBound);
                free((char*)parent_path);
                return name;
            }
            else {
                return get_path_cdir(config, cdir->cdir_parent, false);
            }
        }
        case CNODE_PARENT_PNODE: {
            if(first) {
                char* name;
                const char* parent_path = get_path_pnode(config, cdir->pnode_parent);
                asprintf(&name, "%s_%d_%d", parent_path, cdir->startBound, cdir->endBound);
                free((char*)parent_path);
                return name;
            }
            else {
                return get_path_pnode(config, cdir->pnode_parent);
            }
        }
    }
}

const char* get_path_cnode(const struct config* config, const struct cnode* cnode)
{
    if(cnode->parent == NULL) {
        return strdup("");
    }
    return get_path_cdir(config, cnode->parent, true);
}

const char* get_name_pnode(const struct config* config, const struct pnode* pnode)
{
    char* name;
    const char* path = get_path_pnode(config, pnode);
    asprintf(&name, "pnode_%s", path);
    return name;
}

const char* get_name_cdir(const struct config* config, const struct cdir* cdir)
{
    char* name;
    const char* path = get_path_cdir(config, cdir, true);
    asprintf(&name, "cdir_%s", path);
    return name;
}

const char* get_name_cnode(const struct config* config, const struct cnode* cnode)
{
    if(cnode->parent == NULL) {
        return strdup("cnode_root");
    }
    else {
        char* name;
        const char* path = get_path_cnode(config, cnode);
        asprintf(&name, "cnode_%s", path);
        free((char*)path);
        return name;
    }
}

const char* get_name_lnode(const struct config* config, const struct lnode* lnode)
{
    char* name;
    const char* path = get_path_cnode(config, lnode->parent);
    asprintf(&name, "lnode_%s", path);
    free((char*)path);
    return name;
}

const char* get_name_pdir(const struct config* config, const struct pdir* pdir)
{
    char* name;
    const char* path = get_path_cnode(config, pdir->parent);
    asprintf(&name, "pdir_%s", path);
    free((char*)path);
    return name;
}

void write_dot_file_lnode_names(FILE* f, const struct config* config, const struct lnode* lnode, unsigned int level)
{
    const char* name = get_name_lnode(config, lnode);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "\"%s\" [label=\"l-node\", fillcolor=black, style=filled, fontcolor=white, shape=circle]\n", name);
    free((char*)name);
}

void write_dot_file_cnode_names(FILE* f, const struct config* config, const struct cnode* cnode, unsigned int level);

void write_dot_file_cdir_inner_names(FILE* f, const struct config* config, const struct cdir* cdir, unsigned int level)
{
    const char* name = get_name_cdir(config, cdir);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "\"%s\" [label=\"[%d, %d]\", fillcolor=darkolivegreen3, style=filled, shape=record]\n", name, cdir->startBound, cdir->endBound);
    if(cdir->lChild != NULL) {
        write_dot_file_cdir_inner_names(f, config, cdir->lChild, level);
    }
    if(cdir->rChild != NULL) {
        write_dot_file_cdir_inner_names(f, config, cdir->rChild, level);
    }
}

void write_dot_file_cdir_cnode_names(FILE* f, const struct config* config, const struct cdir* cdir, unsigned int level)
{
    if(cdir->cnode != NULL) {
        write_dot_file_cnode_names(f, config, cdir->cnode, level);
    }
    if(cdir->lChild != NULL) {
        write_dot_file_cdir_cnode_names(f, config, cdir->lChild, level);
    }
    if(cdir->rChild != NULL) {
        write_dot_file_cdir_cnode_names(f, config, cdir->rChild, level);
    }
}

void write_dot_file_cdir_names(FILE* f, const struct config* config, const struct cdir* cdir, unsigned int level)
{
    const char* name = get_name_cdir(config, cdir);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "subgraph \"cluster%s\" {\n", name);
    level++;
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "color=orange; fillcolor=orange; style=filled; label=\"c-directory\"");
    write_dot_file_cdir_inner_names(f, config, cdir, level);
    level--;
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "}\n");
    free((char*)name);
    write_dot_file_cdir_cnode_names(f, config, cdir, level);
}

void write_dot_file_pnode_names(FILE* f, const struct config* config, const struct pnode* pnode, unsigned int level)
{
    const char* name = get_name_pnode(config, pnode);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "\"%s\" [label=\"%s\", color=cyan2, fillcolor=cyan2, style=filled, shape=record]\n", name, attr);
    free((char*)name);
    if(pnode->cdir != NULL) {
        write_dot_file_cdir_names(f, config, pnode->cdir, level);
    }
}

void write_dot_file_pdir_inner_names(FILE* f, const struct config* config, const struct pdir* pdir, unsigned int level)
{
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        const char* name = get_name_pnode(config, pnode);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        const char* attr = get_attr_for_id(config, pnode->variable_id);
        fprintf(f, "\"%s\" [label=\"%s\", color=cyan2, fillcolor=cyan2, style=filled, shape=record]\n", name, attr);
    }
}

void write_dot_file_pdir_names(FILE* f, const struct config* config, const struct pdir* pdir, unsigned int level)
{
    const char* name = get_name_pdir(config, pdir);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "subgraph \"cluster%s\" {\n", name);
    level++;
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "color=lightpink; fillcolor=lightpink; style=filled; label=\"p-directory\"");
    write_dot_file_pdir_inner_names(f, config, pdir, level);
    level--;
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "}\n");
    free((char*)name);
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
        write_dot_file_pnode_names(f, config, pdir->pnodes[i], level);
    }
}

void write_dot_file_cnode_names(FILE* f, const struct config* config, const struct cnode* cnode, unsigned int level)
{
    const char* name = get_name_cnode(config, cnode);
    fprintf(f, "%.*s", level * 4, SEP_SPACE);
    fprintf(f, "\"%s\" [label=\"c-node\", color=darkolivegreen3, fillcolor=darkolivegreen3, style=filled, shape=circle]\n", name);
    free((char*)name);
    if(cnode->lnode != NULL) {
        write_dot_file_lnode_names(f, config, cnode->lnode, level);
    }
    if(cnode->pdir != NULL) {
        write_dot_file_pdir_names(f, config, cnode->pdir, level);
    }
}

void write_dot_file_cnode_links(FILE* f, const struct config* config, const struct cnode* cnode, unsigned int level);

void write_dot_file_cdir_links(FILE* f, const struct config* config, const struct cdir* cdir, unsigned int level)
{
    const char* cdir_name = get_name_cdir(config, cdir);
    if(cdir->cnode != NULL) {
        const char* cnode_name = get_name_cnode(config, cdir->cnode);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\"\n", cdir_name, cnode_name);
        free((char*)cnode_name);
        write_dot_file_cnode_links(f, config, cdir->cnode, level);
    }
    if(cdir->lChild != NULL) {
        const char* lchild_name = get_name_cdir(config, cdir->lChild);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\"\n", cdir_name, lchild_name);
        free((char*)lchild_name);
        write_dot_file_cdir_links(f, config, cdir->lChild, level);
    }
    if(cdir->lChild != NULL) {
        const char* rchild_name = get_name_cdir(config, cdir->rChild);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\"\n", cdir_name, rchild_name);
        free((char*)rchild_name);
        write_dot_file_cdir_links(f, config, cdir->rChild, level);
    }
    free((char*)cdir_name);
}

void write_dot_file_pnode_links(FILE* f, const struct config* config, const struct pnode* pnode, unsigned int level)
{
    if(pnode->cdir != NULL) {
        const char* pnode_name = get_name_pnode(config, pnode);
        const char* cdir_name = get_name_cdir(config, pnode->cdir);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\"\n", pnode_name, cdir_name);
        free((char*)pnode_name);
        free((char*)cdir_name);
        write_dot_file_cdir_links(f, config, pnode->cdir, level);
    }
}

void write_dot_file_pdir_links(FILE* f, const struct config* config, const struct pdir* pdir, unsigned int level)
{
    for(unsigned int i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        write_dot_file_pnode_links(f, config, pnode, level);
    }
}

void write_dot_file_cnode_links(FILE* f, const struct config* config, const struct cnode* cnode, unsigned int level)
{
    const char* cnode_name = get_name_cnode(config, cnode);
    if(cnode->lnode != NULL) {
        const char* lnode_name = get_name_lnode(config, cnode->lnode);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\"\n", cnode_name, lnode_name);
        free((char*)lnode_name);
    }
    if(cnode->pdir != NULL && cnode->pdir->pnode_count > 0) {
        const char* pdir_name = get_name_pdir(config, cnode->pdir);
        const char* pnode_name = get_name_pnode(config, cnode->pdir->pnodes[0]);
        fprintf(f, "%.*s", level * 4, SEP_SPACE);
        fprintf(f, "\"%s\" -> \"%s\" [lhead=\"cluster%s\"]\n", cnode_name, pnode_name, pdir_name);
        free((char*)pdir_name);
        free((char*)pnode_name);
        write_dot_file_pdir_links(f, config, cnode->pdir, level);
    }
    free((char*)cnode_name);
}

void write_dot_file_cnode_ranks(FILE* f, const struct config* config, const struct cnode* cnode, unsigned int level)
{

}

void write_dot_file(const struct config* config, const struct cnode* root)
{
    FILE* f = fopen("betree.dot", "w");
    if(f == NULL) {
        fprintf(stderr, "Can't open a file to write the dot_file");
        exit(1);
    }
    fprintf(f, "digraph {\n");
    fprintf(f, "    compound=true");
    fprintf(f, "    node [fontsize=20; fontname=\"Verdana\"];\n");
    write_dot_file_cnode_names(f, config, root, 1);
    write_dot_file_cnode_links(f, config, root, 1);
    write_dot_file_cnode_ranks(f, config, root, 1);
    fprintf(f, "}\n");
}
