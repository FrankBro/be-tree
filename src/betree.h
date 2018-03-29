#ifndef BETREE_H__
#define BETREE_H__

#include <stdbool.h>
#include <stdlib.h>

struct pred {
    unsigned int variable_id;
    int value;
};

struct sub {
    unsigned int id;
    unsigned int variable_id_count;
    // TODO Make const
    unsigned int* variable_ids;
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
    unsigned int variable_id;
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
    unsigned int variable_id;
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
    unsigned int variable_id;
    int min_bound;
    int max_bound;
};

struct config {
    unsigned int lnode_max_cap;
    unsigned int partition_min_size;
    unsigned int attr_domain_count;
    struct attr_domain** attr_domains;
    unsigned int attr_to_id_count;
    // TODO Make const
    char** attr_to_ids;
};

struct config* make_config(unsigned int lnode_max_cap, unsigned int partition_min_size);
struct config* make_default_config();
void free_config(struct config* config);
void add_attr_domain(struct config* config, const char* attr, int min_bound, int max_bound);

const char* get_attr_for_id(const struct config* config, unsigned int variable_id);
unsigned int get_id_for_attr(struct config* config, const char* attr);

void free_sub(struct sub* sub);
void free_event(struct event* event);

bool sub_has_attribute(const struct sub* sub, unsigned int variable_id);
bool sub_has_attribute_str(struct config* config, const struct sub* sub, const char* attr);
bool sub_is_enclosed(const struct config* config, const struct sub* sub, const struct cdir* cdir);

void insert_sub(const struct sub* sub, struct lnode* lnode);
bool remove_sub(const struct sub* sub, struct lnode* lnode);

struct lnode* make_lnode(const struct config* config, struct cnode* parent);
void free_lnode(struct lnode* lnode);
struct cnode* make_cnode(const struct config* config, struct cdir* parent);
void free_cnode(struct cnode* cnode);

struct matched_subs {
    unsigned int sub_count;
    unsigned int *subs;
};

struct matched_subs* make_matched_subs();
void free_matched_subs(struct matched_subs* matched_subs);
const struct pred* make_simple_pred(unsigned int variable_id, int value);
const struct pred* make_simple_pred_str(struct config* config, const char* attr, int value);
void fill_pred(struct sub* sub, const struct ast_node* expr);
struct sub* make_empty_sub(unsigned int id);
const struct sub* make_sub(struct config* config, unsigned int id, struct ast_node* expr);
const struct event* make_simple_event(struct config* config, const char* attr, int value);

void insert_be_tree(const struct config* config, const struct sub* sub, struct cnode* cnode, struct cdir* cdir);
void match_be_tree(const struct event* event, const struct cnode* cnode, struct matched_subs* matchedSub);
bool delete_be_tree(const struct config* config, struct sub* sub, struct cnode* cnode);

#endif
