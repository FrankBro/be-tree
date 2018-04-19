/*
 * RosettaCode example: Statistics/Normal distribution in C
 *
 * The random number generator rand() of the standard C library is obsolete
 * and should not be used in more demanding applications. There are plenty
 * libraries with advanced features (eg. GSL) with functions to calculate 
 * the mean, the standard deviation, generating random numbers etc. 
 * However, these features are not the core of the standard C library.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "ast.h"
#include "random_words.h"
#include "utils.h"
 
double mean(double* values, size_t n)
{
    double s = 0;
 
    for (size_t i = 0; i < n; i++)
        s += values[i];
    return s / n;
}
 
double stddev(double* values, size_t n)
{
    double average = mean(values,n);
    double s = 0;
 
    for (size_t i = 0; i < n; i++)
        s += (values[i] - average) * (values[i] - average);
    return sqrt(s / (n - 1));
}
 
/*
 * Normal random numbers generator - Marsaglia algorithm.
 */
double* generate(size_t n)
{
    size_t m = n + n % 2;
    double* values = (double*)calloc(m,sizeof(double));
 
    if (values)
    {
        for (size_t i = 0; i < m; i += 2)
        {
            double x, y, rsq, f;
            do {
                x = 2.0 * rand() / (double)RAND_MAX - 1.0;
                y = 2.0 * rand() / (double)RAND_MAX - 1.0;
                rsq = x * x + y * y;
            } while (rsq >= 1. || rsq == 0.);
            f = sqrt( -2.0 * log(rsq) / rsq );
            values[i]   = x * f;
            values[i+1] = y * f;
        }
    }
    return values;
}
 
const struct ast_node* generate_expr(size_t complexity, size_t attr_min, size_t attr_max, int64_t value_min, int64_t value_max)
{
    struct ast_node* last_combi_node;
    for(size_t j = 0; j < complexity; j++) {
        enum ast_binop_e binop = random_in_range(0, 5);
        size_t attr_index = random_in_range(attr_min, attr_max);
        const char* attr = RANDOM_WORDS[attr_index];
        int64_t ivalue = random_in_range(value_min, value_max);
        struct value value = { .value_type = VALUE_I, .ivalue = ivalue };
        struct ast_node* bin_node = ast_binary_expr_create(binop, attr, value);

        enum ast_combi_e combiop = random_in_range(0, 1);
        if(j == 0) {
            size_t another_attr_index = random_in_range(attr_min, attr_max);
            const char* another_attr = RANDOM_WORDS[another_attr_index];
            enum ast_binop_e another_binop = random_in_range(0, 5);
            int64_t another_ivalue = random_in_range(value_min, value_max);
            struct value another_value = { .value_type = VALUE_I, .ivalue = another_ivalue };
            struct ast_node* another_bin_node = ast_binary_expr_create(another_binop, another_attr, another_value);
            last_combi_node = ast_combi_expr_create(combiop, bin_node, another_bin_node);
        }
        else {
            last_combi_node = ast_combi_expr_create(combiop, bin_node, last_combi_node);
        }
    }
    return last_combi_node;
}

void write_expr(FILE* f, const struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_LIST_EXPR): {
            const char* integer_list = integer_list_to_string(node->list_expr.list);
            switch(node->list_expr.op) {
                case AST_LISTOP_IN: {
                    fprintf(f, "%s in (%s)", node->list_expr.name, integer_list);
                    break;
                }
                case AST_LISTOP_NOTIN: {
                    fprintf(f, "%s not in (%s)", node->list_expr.name, integer_list);
                    break;
                }
            }
            free((char*)integer_list);
            break;
        }
        case(AST_TYPE_BINARY_EXPR): {
            fprintf(f, "%s ", node->binary_expr.name);
            switch(node->binary_expr.op) {
                case AST_BINOP_LT: {
                    fprintf(f, "< ");
                    break;
                }
                case AST_BINOP_LE: {
                    fprintf(f, "<= ");
                    break;
                }
                case AST_BINOP_EQ: {
                    fprintf(f, "= ");
                    break;
                }
                case AST_BINOP_NE: {
                    fprintf(f, "<> ");
                    break;
                }
                case AST_BINOP_GT: {
                    fprintf(f, "> ");
                    break;
                }
                case AST_BINOP_GE: {
                    fprintf(f, ">= ");
                    break;
                }
            }
            fprintf(f, "%llu ", node->binary_expr.value.ivalue);
            break;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_NONE: {
                    fprintf(f, "%s", node->bool_expr.name);
                    break;
                }
                case AST_BOOL_NOT: {
                    fprintf(f, "not %s", node->bool_expr.name);
                    break;
                }
            }
            break;
        }
        case(AST_TYPE_COMBI_EXPR): {
            write_expr(f, node->combi_expr.lhs);
            switch(node->combi_expr.op) {
                case AST_COMBI_AND: {
                    fprintf(f, "&& ");
                    break;
                }
                case AST_COMBI_OR: {
                    fprintf(f, "|| ");
                    break;
                }
            }
            write_expr(f, node->combi_expr.rhs);
            break;
        }
    }
}

int main(void)
{
    unsigned int expr_count = 1000;
    unsigned int attr_min = 0;
    unsigned int attr_max = 499;
    unsigned int value_min = 0;
    unsigned int value_max = 100;
    unsigned int complexity_min = 5;
    unsigned int complexity_max = 55;
    unsigned int complexity_mean = (complexity_min + complexity_max) / 2;
    double complexity_stddev = (complexity_max - complexity_mean) / 3;
 
    srand((unsigned int)time(NULL));

    FILE* f = fopen("output.be", "w");

    double* seq = generate(expr_count);
    for(unsigned int i = 0; i < expr_count; i++) {
        unsigned int complexity = seq[i] * complexity_stddev + complexity_mean;
        const struct ast_node* node = generate_expr(complexity, attr_min, attr_max, value_min, value_max);
        write_expr(f, node);
        fprintf(f, "\n");
        free_ast_node((struct ast_node*)node);
    }

    fclose(f);

    free(seq);
    return 0;
}