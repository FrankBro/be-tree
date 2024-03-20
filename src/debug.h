#pragma once

struct config;
struct cnode;
struct betree;

void write_dot_file(const struct betree* tree);
void write_dot_to_file(const struct betree* tree, const char* fname);
