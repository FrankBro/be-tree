#pragma once

struct config;
struct cnode;
struct betree;

void print_be_tree(const struct config* config, const struct cnode* root);

void write_dot_file(const struct config* config, const struct cnode* root);
void write_dot_file_tree(const struct betree* tree);

