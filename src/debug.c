#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "betree.h"
#include "utils.h"

#define SEP_DASH  "------------------------------------------------------"
#define SEP_SPACE "                                                      "

void print_dashs(uint64_t level) {
    printf("%.*s", (int)level * 2, SEP_DASH);
}

void print_lnode(const struct lnode* lnode, uint64_t level)
{
    if(lnode == NULL) {
        return;
    }
    print_dashs(level);
    printf(" lnode (%zu) [", lnode->max);
    for(size_t i = 0; i < lnode->sub_count; i++) {
        printf("%llu", lnode->subs[i]->id);
        if(i != lnode->sub_count - 1) {
            printf(", ");
        }
    }
    printf("]");
}

void print_cnode(const struct config* config, const struct cnode* cnode, uint64_t level);

void print_cdir(const struct config* config, const struct cdir* cdir, uint64_t level)
{
    if(cdir == NULL) {
        return;
    }
    print_dashs(level);
    switch(cdir->bound.value_type) {
        case(VALUE_I): {
            printf(" cdir [%llu, %llu]", cdir->bound.imin, cdir->bound.imax);
            break;
        }
        case(VALUE_F): {
            printf(" cdir [%.2f, %.2f]", cdir->bound.fmin, cdir->bound.fmax);
            break;
        }
        case(VALUE_B): {
            const char* min = cdir->bound.bmin ? "true" : "false";
            const char* max = cdir->bound.bmax ? "true" : "false";
            printf(" cdir [%s, %s]", min, max);
            break;
        }
        case(VALUE_S): {
            fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
            abort();
        }
        case(VALUE_IL): {
            fprintf(stderr, "%s a integer list value cdir should never happen for now", __func__);
            abort();
        }
        case(VALUE_SL): {
            fprintf(stderr, "%s a string list value cdir should never happen for now", __func__);
            abort();
        }
        default: {
            switch_default_error("Invalid bound value type");
        }
    }
    if(cdir->cnode != NULL) {
        printf("\n");
        print_cnode(config, cdir->cnode, level + 1);
    }
    if(cdir->lchild != NULL) {
        printf("\n");
        print_cdir(config, cdir->lchild, level + 1);
    }
    if(cdir->rchild != NULL) {
        printf("\n");
        print_cdir(config, cdir->rchild, level + 1);
    }
}

void print_pnode(const struct config* config, const struct pnode* pnode, uint64_t level)
{
    if(pnode == NULL) {
        return;
    }
    print_dashs(level);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    printf(" pnode %s (%f)", attr, pnode->score);
    if(pnode->cdir != NULL) {
        printf("\n");
        print_cdir(config, pnode->cdir, level + 1);
    }
}

void print_pdir(const struct config* config, const struct pdir* pdir, uint64_t level)
{
    if(pdir == NULL) {
        return;
    }
    print_dashs(level);
    printf(" pdir");
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        if(pnode != NULL) {
            printf("\n");
            print_pnode(config, pnode, level + 1);
        }
    }
}

void print_cnode(const struct config* config, const struct cnode* cnode, uint64_t level)
{
    if(cnode == NULL) {
        return;
    }
    print_dashs(level);
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

const char* escape_name(const char* input)
{
    size_t len = strlen(input);
    char* escaped = calloc(len + 1, sizeof(*escaped));
    if(escaped == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    for(size_t i = 0; i < len; i++) {
        if(input[i] == '-') {
            escaped[i] = '_';
        }
        else {
            escaped[i] = input[i];
        }
    }
    escaped[len] = '\0';
    return escaped;
}

const char* get_path_pnode(const struct config* config, const struct pnode* pnode)
{
    char* name;
    const char* parent_path = get_path_cnode(config, pnode->parent->parent);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    const char* escaped_attr = escape_name(attr);
    asprintf(&name, "%s_%s", parent_path, escaped_attr);
    free((char*)parent_path);
    free((char*)escaped_attr);
    return name;
}

const char* get_path_cdir(const struct config* config, const struct cdir* cdir, bool first)
{
    const char* parent_path;
    switch(cdir->parent_type) {
        case(CNODE_PARENT_CDIR): {
            if(first) {
                parent_path = get_path_cdir(config, cdir->cdir_parent, false);
                break;
            }
            else {
                return get_path_cdir(config, cdir->cdir_parent, false);
            }
        }
        case(CNODE_PARENT_PNODE): {
            if(first) {
                parent_path = get_path_pnode(config, cdir->pnode_parent);
                break;
            }
            else {
                return get_path_pnode(config, cdir->pnode_parent);
            }
        }
        default: {
            switch_default_error("Invalid cdir parent type");
        }
    }
    char* name;
    switch(cdir->bound.value_type) {
        case(VALUE_I): {
            asprintf(&name, "%s_%llu_%llu", parent_path, cdir->bound.imin, cdir->bound.imax);
            break;
        }
        case(VALUE_F): {
            asprintf(&name, "%s_%.0f_%.0f", parent_path, cdir->bound.fmin, cdir->bound.fmax);
            break;
        }
        case(VALUE_B): {
            const char* min = cdir->bound.bmin ? "true" : "false";
            const char* max = cdir->bound.bmax ? "true" : "false";
            asprintf(&name, "%s_%s_%s", parent_path, min, max);
            break;
        }
        case(VALUE_S): {
            fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
            abort();
        }
        case(VALUE_IL): {
            fprintf(stderr, "%s a integer list value cdir should never happen for now", __func__);
            abort();
        }
        case(VALUE_SL): {
            fprintf(stderr, "%s a string list value cdir should never happen for now", __func__);
            abort();
        }
        default: {
            switch_default_error("Invalid bound value type");
        }
    }
    free((char*)parent_path);
    return name;
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
    free((char*)path);
    return name;
}

const char* get_name_cdir(const struct config* config, const struct cdir* cdir)
{
    char* name;
    const char* path = get_path_cdir(config, cdir, true);
    asprintf(&name, "cdir_%s", path);
    free((char*)path);
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

void print_spaces(FILE* f, uint64_t level) {
    fprintf(f, "%.*s", (int)level * 4, SEP_SPACE);
}

void write_dot_file_lnode_names(FILE* f, const struct config* config, const struct lnode* lnode, uint64_t level)
{
    const char* name = get_name_lnode(config, lnode);
    print_spaces(f, level);
    fprintf(f, "\"%s\" [label=\"l-node\", fillcolor=black, style=filled, fontcolor=white, shape=circle, fixedsize=true, width=0.8]\n", name);
    if(lnode->sub_count > 0) {
        print_spaces(f, level);
        fprintf(f, "\"%s_subs\" [label=<\\\{", name);
        for(size_t i = 0; i < lnode->sub_count; i++) {
            if(i != 0) {
                fprintf(f, ", ");
            }
            fprintf(f, "S<sub>%llu</sub>", lnode->subs[i]->id);
        }
        fprintf(f, "\\}>, color=lightblue1, fillcolor=lightblue1, style=filled, shape=record]\n");
    }
    free((char*)name);
}

void write_dot_file_cnode_names(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level);

uint64_t colspan_value(uint64_t level) {
    if(level == 0) {
        return 1;
    }
    return colspan_value(level - 1) * 2;
}

void write_dot_file_cdir_td(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level, size_t current_depth, uint64_t colspan)
{
    const struct cdir* left = cdir != NULL ? cdir->lchild : NULL;
    const struct cdir* right = cdir != NULL ? cdir->rchild : NULL;
    
    if(current_depth == 0) {
        print_spaces(f, level);
        if(cdir == NULL) {
            fprintf(f, "<td colspan=\"%llu\"></td>\n", colspan);
        }
        else {
            const char* name = get_name_cdir(config, cdir);
            switch(cdir->bound.value_type) {
                case(VALUE_I): {
                    fprintf(f, "<td colspan=\"%llu\" port=\"%s\">[%llu, %llu]</td>\n", colspan, name, cdir->bound.imin, cdir->bound.imax);
                    break;
                }
                case(VALUE_F): {
                    fprintf(f, "<td colspan=\"%llu\" port=\"%s\">[%.0f, %.0f]</td>\n", colspan, name, cdir->bound.fmin, cdir->bound.fmax);
                    break;
                }
                case(VALUE_B): {
                    const char* min = cdir->bound.bmin ? "true" : "false";
                    const char* max = cdir->bound.bmax ? "true" : "false";
                    fprintf(f, "<td colspan=\"%llu\" port=\"%s\">[%s, %s]</td>\n", colspan, name, min, max);
                    break;
                }
                case(VALUE_S): {
                    fprintf(stderr, "%s a string value cdir should never happen for now", __func__);
                    abort();
                }
                case(VALUE_IL): {
                    fprintf(stderr, "%s a integer list value cdir should never happen for now", __func__);
                    abort();
                }
                case(VALUE_SL): {
                    fprintf(stderr, "%s a string list value cdir should never happen for now", __func__);
                    abort();
                }
                default: {
                    switch_default_error("Invalid bound value type");
                }
            }
            free((char*)name);
        }
    }
    else {
        write_dot_file_cdir_td(f, config, left, level, current_depth - 1, colspan);
        write_dot_file_cdir_td(f, config, right, level, current_depth - 1, colspan);
    }
}

void write_dot_file_cdir_inner_names(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level, size_t depth_of_cdir)
{
    for(size_t current_depth = 0; current_depth < depth_of_cdir; current_depth++) {
        print_spaces(f, level);
        fprintf(f, "<tr>\n");
        level++;
        uint64_t colspan = colspan_value(depth_of_cdir - current_depth) / 2;
        write_dot_file_cdir_td(f, config, cdir, level, current_depth, colspan);
        level--;
        print_spaces(f, level);
        fprintf(f, "</tr>\n");
    }
}

void write_dot_file_cdir_cnode_names(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level)
{
    if(cdir->cnode != NULL) {
        write_dot_file_cnode_names(f, config, cdir->cnode, level);
    }
    if(cdir->lchild != NULL) {
        write_dot_file_cdir_cnode_names(f, config, cdir->lchild, level);
    }
    if(cdir->rchild != NULL) {
        write_dot_file_cdir_cnode_names(f, config, cdir->rchild, level);
    }
}

void write_dot_file_cdir_cdir_ranks(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level);

size_t depth_of_cdir(const struct cdir* cdir)
{
    if(cdir == NULL) {
        return 0;
    }
    size_t current = 1;
    const struct cdir* parent = cdir->parent_type == CNODE_PARENT_CDIR ? cdir->cdir_parent : NULL;
    while(parent != NULL) {
        current++;
        if(parent->parent_type == CNODE_PARENT_PNODE) {
            break;
        }
        parent = parent->cdir_parent;
    }
    size_t left = depth_of_cdir(cdir->lchild);
    size_t right = depth_of_cdir(cdir->rchild);
    size_t ret = max(left, right);
    return max(ret, current);
}

void write_dot_file_cdir_names(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level)
{
    const char* name = get_name_cdir(config, cdir);
    print_spaces(f, level);
    fprintf(f, "subgraph \"cluster%s\" {\n", name);
    level++;
    print_spaces(f, level);
    fprintf(f, "color=orange; fillcolor=orange; style=filled; label=\"c-directory\"; fontsize=20; fontname=\"Verdana\"\n");
    print_spaces(f, level);
    fprintf(f, "%s [fillcolor=darkolivegreen3, style=filled, shape=box, label=<\n", name);
    level++;
    print_spaces(f, level);
    fprintf(f, "<table>\n");
    level++;
    size_t depth = depth_of_cdir(cdir);
    write_dot_file_cdir_inner_names(f, config, cdir, level, depth);
    level--;
    print_spaces(f, level);
    fprintf(f, "</table>\n");
    level--;
    print_spaces(f, level);
    fprintf(f, ">]\n");
    level--;
    print_spaces(f, level);
    fprintf(f, "}\n");
    free((char*)name);
    write_dot_file_cdir_cnode_names(f, config, cdir, level);
}

void write_dot_file_pnode_names(FILE* f, const struct config* config, const struct pnode* pnode, uint64_t level)
{
    const char* name = get_name_pnode(config, pnode);
    const char* attr = get_attr_for_id(config, pnode->variable_id);
    print_spaces(f, level);
    fprintf(f, "\"%s\" [label=\"%s\", color=cyan2, fillcolor=cyan2, style=filled, shape=record]\n", name, attr);
    print_spaces(f, level);
    fprintf(f, "\"%s_fake\" [label=\"p-node\", color=cyan2, fillcolor=cyan2, style=filled, shape=circle, fixedsize=true, width=0.8]\n", name);
    free((char*)name);
    if(pnode->cdir != NULL) {
        write_dot_file_cdir_names(f, config, pnode->cdir, level);
    }
}

void write_dot_file_pdir_inner_names(FILE* f, const struct config* config, const struct pdir* pdir, uint64_t level)
{
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        const char* name = get_name_pnode(config, pnode);
        const char* attr = get_attr_for_id(config, pnode->variable_id);
        print_spaces(f, level);
        fprintf(f, "\"%s\" [label=\"%s\", color=cyan2, fillcolor=cyan2, style=filled, shape=record]\n", name, attr);
        free((char*)name);
    }
}

void write_dot_file_pdir_names(FILE* f, const struct config* config, const struct pdir* pdir, uint64_t level)
{
    const char* name = get_name_pdir(config, pdir);
    print_spaces(f, level);
    fprintf(f, "subgraph \"cluster%s\" {\n", name);
    level++;
    print_spaces(f, level);
    fprintf(f, "color=lightpink; fillcolor=lightpink; style=filled; label=\"p-directory\"; fontsize=20; fontname=\"Verdana\"\n");
    write_dot_file_pdir_inner_names(f, config, pdir, level);
    level--;
    print_spaces(f, level);
    fprintf(f, "}\n");
    free((char*)name);
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        write_dot_file_pnode_names(f, config, pdir->pnodes[i], level);
    }
}

void write_dot_file_cnode_names(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level)
{
    const char* name = get_name_cnode(config, cnode);
    print_spaces(f, level);
    fprintf(f, "\"%s\" [label=\"c-node\", color=darkolivegreen3, fillcolor=darkolivegreen3, style=filled, shape=circle, fixedsize=true, width=0.8]\n", name);
    free((char*)name);
    if(cnode->lnode != NULL) {
        write_dot_file_lnode_names(f, config, cnode->lnode, level);
    }
    if(cnode->pdir != NULL) {
        write_dot_file_pdir_names(f, config, cnode->pdir, level);
    }
}

void write_dot_file_cnode_links(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level);

void write_dot_file_cdir_links(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level, const char* table_name)
{
    const char* cdir_name = get_name_cdir(config, cdir);
    if(cdir->cnode != NULL) {
        const char* cnode_name = get_name_cnode(config, cdir->cnode);
        print_spaces(f, level);
        fprintf(f, "%s:%s -> \"%s\"\n", table_name, cdir_name, cnode_name);
        free((char*)cnode_name);
        write_dot_file_cnode_links(f, config, cdir->cnode, level);
    }
    if(cdir->lchild != NULL) {
        write_dot_file_cdir_links(f, config, cdir->lchild, level, table_name);
    }
    if(cdir->rchild != NULL) {
        write_dot_file_cdir_links(f, config, cdir->rchild, level, table_name);
    }
    free((char*)cdir_name);
}

void write_dot_file_pnode_links(FILE* f, const struct config* config, const struct pnode* pnode, uint64_t level)
{
    if(pnode->cdir != NULL) {
        const char* pnode_name = get_name_pnode(config, pnode);
        const char* cdir_name = get_name_cdir(config, pnode->cdir);
        print_spaces(f, level);
        fprintf(f, "\"%s\" -> \"%s_fake\"\n", pnode_name, pnode_name);
        print_spaces(f, level);
        fprintf(f, "\"%s_fake\" -> \"%s\" [lhead=\"cluster%s\"]\n", pnode_name, cdir_name, cdir_name);
        free((char*)pnode_name);
        write_dot_file_cdir_links(f, config, pnode->cdir, level, cdir_name);
        free((char*)cdir_name);
    }
}

void write_dot_file_pdir_links(FILE* f, const struct config* config, const struct pdir* pdir, uint64_t level)
{
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        const struct pnode* pnode = pdir->pnodes[i];
        write_dot_file_pnode_links(f, config, pnode, level);
    }
}

void write_dot_file_cnode_links(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level)
{
    const char* cnode_name = get_name_cnode(config, cnode);
    if(cnode->lnode != NULL) {
        const char* lnode_name = get_name_lnode(config, cnode->lnode);
        print_spaces(f, level);
        fprintf(f, "\"%s\" -> \"%s\"\n", cnode_name, lnode_name);
        if(cnode->lnode->sub_count > 0) {
            print_spaces(f, level);
            fprintf(f, "\"%s\" -> \"%s_subs\"\n", lnode_name, lnode_name);
        }
        free((char*)lnode_name);
    }
    if(cnode->pdir != NULL && cnode->pdir->pnode_count > 0) {
        const char* pdir_name = get_name_pdir(config, cnode->pdir);
        const char* pnode_name = get_name_pnode(config, cnode->pdir->pnodes[0]);
        print_spaces(f, level);
        fprintf(f, "\"%s\" -> \"%s\" [lhead=\"cluster%s\"]\n", cnode_name, pnode_name, pdir_name);
        free((char*)pdir_name);
        free((char*)pnode_name);
        write_dot_file_pdir_links(f, config, cnode->pdir, level);
    }
    free((char*)cnode_name);
}

void write_dot_file_cdir_cnode_ranks(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level, bool first)
{
    if(cdir->cnode != NULL) {
        const char* cnode_name = get_name_cnode(config, cdir->cnode);
        if(!first) {
            fprintf(f, ", ");
        }
        fprintf(f, "\"%s\"", cnode_name);
        free((char*)cnode_name);
    }
    if(cdir->lchild != NULL) {
        write_dot_file_cdir_cnode_ranks(f, config, cdir->lchild, level, false);
    }
    if(cdir->rchild != NULL) {
        write_dot_file_cdir_cnode_ranks(f, config, cdir->rchild, level, false);
    }
}

struct cdir_acc {
    size_t count;
    struct cdir** cdirs;
};

void add_cdir(const struct cdir* cdir, struct cdir_acc* acc)
{
    if(acc->count == 0) {
        acc->cdirs = calloc(1, sizeof(*acc->cdirs));
        if(acc->cdirs == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct cdir** next_cdirs = realloc(acc->cdirs, sizeof(*next_cdirs) * (acc->count + 1));
        if(next_cdirs == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        acc->cdirs = next_cdirs;
    }
    acc->cdirs[acc->count] = (struct cdir*)cdir;
    acc->count++;
}

void get_inner_cdir(const struct cdir* cdir, size_t remaining, struct cdir_acc* acc) {
    if(remaining == 0) {
        add_cdir(cdir, acc);
    }
    else {
        if(cdir->lchild != NULL) {
            get_inner_cdir(cdir->lchild, remaining - 1, acc);
        }
        if(cdir->rchild != NULL) {
            get_inner_cdir(cdir->rchild, remaining - 1, acc);
        }
    }
}

void write_dot_file_cdir_cdir_ranks(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level)
{
    size_t remaining = 0;
    while(true) {
        struct cdir_acc acc = { .count = 0, .cdirs = NULL };
        get_inner_cdir(cdir, remaining, &acc);
        if(acc.count > 0) {
            print_spaces(f, level);
            fprintf(f, "{ rank=same; ");
            for(size_t i = 0; i < acc.count; i++) {
                const char* cdir_name = get_name_cdir(config, acc.cdirs[i]);
                if(i != 0) {
                    fprintf(f, ", ");
                }
                fprintf(f, "\"%s\"", cdir_name);
            }
            fprintf(f, " }\n");
            remaining++;
        }
        else {
            free(acc.cdirs);
            return;
        }
    }
}

void write_dot_file_cnode_ranks(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level);

void write_dot_file_cdir_ranks(FILE* f, const struct config* config, const struct cdir* cdir, uint64_t level, bool first)
{
    if(first) {
        print_spaces(f, level);
        fprintf(f, "{ rank=same; ");
        write_dot_file_cdir_cnode_ranks(f, config, cdir, level, true);
        fprintf(f, " }\n");
    }
    if(cdir->cnode != NULL) {
        write_dot_file_cnode_ranks(f, config, cdir->cnode, level);
    }
    if(cdir->lchild != NULL) {
        write_dot_file_cdir_ranks(f, config, cdir->lchild, level, false);
    }
    if(cdir->rchild != NULL) {
        write_dot_file_cdir_ranks(f, config, cdir->rchild, level, false);
    }
}

void write_dot_file_pnode_ranks(FILE* f, const struct config* config, const struct pnode* pnode, uint64_t level)
{
    if(pnode->cdir != NULL) {
        write_dot_file_cdir_ranks(f, config, pnode->cdir, level, true);
    }
}

void write_dot_file_pdir_ranks(FILE* f, const struct config* config, const struct pdir* pdir, uint64_t level)
{
    print_spaces(f, level);
    fprintf(f, "{ rank=same; ");
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        const char* pnode_name = get_name_pnode(config, pdir->pnodes[i]);
        if(i != 0) {
            fprintf(f, ", ");
        }
        fprintf(f, "\"%s_fake\"", pnode_name);
        free((char*)pnode_name);
    }
    fprintf(f, " }\n");
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        write_dot_file_pnode_ranks(f, config, pdir->pnodes[i], level);
    }
}

void write_dot_file_cnode_ranks(FILE* f, const struct config* config, const struct cnode* cnode, uint64_t level)
{
    if(cnode->pdir != NULL) {
        write_dot_file_pdir_ranks(f, config, cnode->pdir, level);
    }
}

struct gathered_subs {
    size_t count;
    struct sub** subs;
};

void add_sub(const struct sub* sub, struct gathered_subs* gatherer)
{
    if(gatherer->count == 0) {
        gatherer->subs = calloc(1, sizeof(*gatherer->subs));
        if(gatherer->subs == NULL) {
            fprintf(stderr, "%s calloc failed", __func__);
            abort();
        }
    }
    else {
        struct sub** subs = realloc(gatherer->subs, sizeof(*subs) * (gatherer->count + 1));
        if(subs == NULL) {
            fprintf(stderr, "%s realloc failed", __func__);
            abort();
        }
        gatherer->subs = subs;
    }
    gatherer->subs[gatherer->count] = (struct sub*)sub;
    gatherer->count++;
}

void gather_subs_cnode(const struct cnode* cnode, struct gathered_subs* gatherer);

void gather_subs_cdir(const struct cdir* cdir, struct gathered_subs* gatherer)
{
    if(cdir->cnode != NULL) {
        gather_subs_cnode(cdir->cnode, gatherer);
    }
    if(cdir->lchild != NULL) {
        gather_subs_cdir(cdir->lchild, gatherer);
    }
    if(cdir->rchild != NULL) {
        gather_subs_cdir(cdir->rchild, gatherer);
    }
}

void gather_subs_pnode(const struct pnode* pnode, struct gathered_subs* gatherer)
{
    if(pnode->cdir != NULL) {
        gather_subs_cdir(pnode->cdir, gatherer);
    }
}

void gather_subs_pdir(const struct pdir* pdir, struct gathered_subs* gatherer)
{
    for(size_t i = 0; i < pdir->pnode_count; i++) {
        gather_subs_pnode(pdir->pnodes[i], gatherer);
    }
}

void gather_subs_cnode(const struct cnode* cnode, struct gathered_subs* gatherer)
{
    for(size_t i = 0; i < cnode->lnode->sub_count; i++) {
        add_sub(cnode->lnode->subs[i], gatherer);
    }
    if(cnode->pdir != NULL) {
        gather_subs_pdir(cnode->pdir, gatherer);
    }
}

int compare_subs(const void* a, const void* b)
{
    struct sub* sub_a = *(struct sub**)a;
    struct sub* sub_b = *(struct sub**)b;

    if(sub_a->id == sub_b->id) return 0;
    else if (sub_a->id < sub_b->id) return -1;
    else return 1;
}

const char* escape_label(const char* input)
{
    // Worst case scenario allocation
    size_t len = strlen(input);
    char* escaped = calloc(len * 2 + 1, sizeof(*escaped));
    if(escaped == NULL) {
        fprintf(stderr, "%s calloc failed", __func__);
        abort();
    }
    size_t j = 0;
    for(size_t i = 0; i < len; i++) {
        if(input[i] == '<' || input[i] == '|' || input[i] == '>') {
            escaped[j] = '\\';
            j++;
        }
        escaped[j] = input[i];
        j++;
    }
    escaped[j] = '\0';
    return escaped;
}

void write_dot_file_root_subs(FILE* f, const struct cnode* cnode, uint64_t level)
{
    print_spaces(f, level);
    fprintf(f, "subgraph \"clustersubs\" {\n");
    level++;
    print_spaces(f, level);
    fprintf(f, "color=lightblue1; fillcolor=lightblue1; style=filled; label=\"Subs\"; fontsize=20; fontname=\"Verdana\"\n");
    struct gathered_subs gatherer = { .count = 0, .subs = NULL };
    gather_subs_cnode(cnode, &gatherer);
    qsort(gatherer.subs, gatherer.count, sizeof(struct sub*), compare_subs);
    for(size_t i = 0; i < gatherer.count; i++) {
        const struct sub* sub = gatherer.subs[i];
        const char* expr = ast_to_string(sub->expr);
        const char* escaped = escape_label(expr);
        print_spaces(f, level);
        fprintf(f, "\"sub_%llu\" [label=\"%s\", color=lightblue1, fillcolor=lightblue1, style=filled, shape=record]\n", sub->id, escaped);
        free((char*)expr);
        free((char*)escaped);
    }
    level--;
    print_spaces(f, level);
    fprintf(f, "}\n");
}

void write_dot_file(const struct config* config, const struct cnode* root)
{
    FILE* f = fopen("betree.dot", "w");
    if(f == NULL) {
        fprintf(stderr, "Can't open a file to write the dot_file");
        abort();
    }
    fprintf(f, "digraph {\n");
    fprintf(f, "    compound=true");
    fprintf(f, "    node [fontsize=20, fontname=\"Verdana\"];\n");
    write_dot_file_cnode_names(f, config, root, 1);
    // write_dot_file_root_subs(f, config, root, 1);
    write_dot_file_cnode_links(f, config, root, 1);
    write_dot_file_cnode_ranks(f, config, root, 1);
    fprintf(f, "}\n");
}
