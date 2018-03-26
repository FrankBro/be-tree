#ifndef BETREE_H__
#define BETREE_H__

#include <stdbool.h>
#include <stdlib.h>

struct pred {
    const char *attr;
    int value;
};

struct sub {
    int id;
    unsigned int pred_count;
    // TODO Make const
    struct pred** preds;
    const struct ast_node *expr;
};

struct event {
    unsigned int pred_count;
    // TODO Make const
    struct pred** preds;
};

struct cnode;

struct lnode {
    struct cnode* parent;
    unsigned int sub_count;
    // TODO Make const
    struct sub **subs;
    unsigned int max;
};

struct pdir;
struct pnode;
struct cdir;

struct cnode {
    struct cdir* parent;
    struct lnode *lnode;
    struct pdir *pdir;
};

struct cdir;
struct pdir;

struct pnode {
    struct pdir* parent;
    const char* attr;
    struct cdir *cdir;
    float score;
};

enum c_parent_e {
    CNODE_PARENT_PNODE,
    CNODE_PARENT_CDIR,
};

struct cdir {
    enum c_parent_e parent_type;
    union {
        struct pnode* pnode_parent;
        struct cdir* cdir_parent;
    };
    const char *attr;
    int startBound;
    int endBound;
    struct cnode *cnode;
    struct cdir *lChild;
    struct cdir *rChild;
};

struct pdir {
    struct cnode* parent;
    unsigned int pnode_count;
    struct pnode** pnodes;
};

struct attr_domain {
    const char *name;
    int minBound;
    int maxBound;
};

struct config {
    unsigned int lnode_max_cap;
    unsigned int partition_min_size;
    unsigned int attr_domain_count;
    struct attr_domain* attr_domains;
};

void free_sub(struct sub* sub);
void free_event(struct event* event);

bool sub_has_attribute(const struct sub* sub, const char* attr);
bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir);

void insert_sub(const struct sub* sub, struct lnode* lnode);
bool remove_sub(const struct sub* sub, struct lnode* lnode);

struct lnode* make_lnode(const struct config* config, struct cnode* parent);
void free_lnode(struct lnode* lnode);
struct cnode* make_cnode(const struct config* config, struct cdir* parent);
void free_cnode(struct cnode* cnode);

struct matched_subs {
    unsigned int sub_count;
    int *subs;
};

struct matched_subs* make_matched_subs();
void free_matched_subs(struct matched_subs* matched_subs);

void insert_be_tree(const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);
void match_be_tree(const struct event* event, const struct cnode* cnode, struct matched_subs* matchedSub);
bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode);

#endif
