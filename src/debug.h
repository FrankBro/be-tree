#pragma once

struct config;
struct cnode;
struct betree;

void print_be_tree(const struct betree* tree);

void write_dot_file(const struct betree* tree);

