#ifndef DEBUG_H__
#define DEBUG_H__

void print_be_tree(const struct config* config, const struct cnode* root);

void write_dot_file(const struct config* config, const struct cnode* root);

#endif