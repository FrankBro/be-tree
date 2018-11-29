#include <string.h>
#include <float.h>
#include <stdio.h>

#include "alloc.h"
#include "hashmap.h"
#include "tree.h"

void add_variable_from_string(struct betree* betree, const char* line)
{
    struct config* config = betree->config;
    char* domain_copy = bstrdup(line);
    char* line_rest = domain_copy;
    const char* name = strtok_r(line_rest, "|", &line_rest);
    const char* type = strtok_r(line_rest, "|", &line_rest);
    bool allow_undefined = strcmp(strtok_r(line_rest, "|\n", &line_rest), "true") == 0;
    const char* min_str = strtok_r(line_rest, "|\n", &line_rest);
    const char* max_str = strtok_r(line_rest, "|\n", &line_rest);
    if(strcmp(type, "integer") == 0) {
        int64_t min = INT64_MIN, max = INT64_MAX;
        if(min_str != NULL) {
            min = strtoll(min_str, NULL, 10);
        }
        if(max_str != NULL) {
            max = strtoll(max_str, NULL, 10);
        }
        add_attr_domain_bounded_i(config, name, allow_undefined, min, max);
    }
    else if(strcmp(type, "float") == 0) {
        double min = -DBL_MAX, max = DBL_MAX;
        if(min_str != NULL) {
            min = atof(min_str);
        }
        if(max_str != NULL) {
            max = atof(max_str);
        }
        add_attr_domain_bounded_f(config, name, allow_undefined, min, max);
    }
    else if(strcmp(type, "boolean") == 0) {
        add_attr_domain_b(config, name, allow_undefined);
    }
    else if(strcmp(type, "frequency") == 0) {
        add_attr_domain_frequency(config, name, allow_undefined);
    }
    else if(strcmp(type, "segments") == 0) {
        add_attr_domain_segments(config, name, allow_undefined);
    }
    else if(strcmp(type, "string") == 0) {
        if(min_str != NULL) {
            size_t max = atoi(min_str);
            add_attr_domain_bounded_s(config, name, allow_undefined, max);
        }
        else {
            add_attr_domain_s(config, name, allow_undefined);
        }
    }
    else if(strcmp(type, "integer enum") == 0) {
        if(min_str != NULL) {
            size_t max = atoi(min_str);
            add_attr_domain_bounded_ie(config, name, allow_undefined, max);
        }
        else {
            add_attr_domain_ie(config, name, allow_undefined);
        }
    }
    else if(strcmp(type, "integer list") == 0) {
        int64_t min = INT64_MIN, max = INT64_MAX;
        if(min_str != NULL) {
            min = strtoll(min_str, NULL, 10);
        }
        if(max_str != NULL) {
            max = strtoll(max_str, NULL, 10);
        }
        if(min_str != NULL && max_str != NULL) {
            add_attr_domain_bounded_il(config, name, allow_undefined, min, max);
        }
        else {
            add_attr_domain_il(config, name, allow_undefined);
        }
    }
    else if(strcmp(type, "string list") == 0) {
        if(min_str != NULL) {
            size_t max = atoi(min_str);
            add_attr_domain_bounded_sl(config, name, allow_undefined, max);
        }
        else {
            add_attr_domain_sl(config, name, allow_undefined);
        }
    }
    else {
        fprintf(stderr, "Unknown definition type");
        abort();
    }
    bfree(domain_copy);
}

void empty_tree(struct betree* betree)
{
    if(betree->cnode != NULL) {
        free_cnode(betree->cnode);
        betree->cnode = make_cnode(betree->config, NULL);
        free_pred_map(betree->config->pred_map);
        betree->config->pred_map = make_pred_map();
    }
}

