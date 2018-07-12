/*
 * RosettaCode example: Statistics/Normal distribution in C
 *
 * The random number generator rand() of the standard C library is obsolete
 * and should not be used in more demanding applications. There are plenty
 * libraries with advanced features (eg. GSL) with functions to calculate 
 * the mean, the standard deviation, generating random numbers etc. 
 * However, these features are not the core of the standard C library.
 */
#include <inttypes.h>
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
            } while (rsq >= 1. || feq(rsq, 0.));
            f = sqrt( -2.0 * log(rsq) / rsq );
            values[i]   = x * f;
            values[i+1] = y * f;
        }
    }
    return values;
}

const struct ast_node* make_binary_expr(size_t attr_min, size_t attr_max, int64_t value_min, int64_t value_max) 
{
    struct ast_node* binary_node;
    size_t attr_index = random_in_range(attr_min, attr_max);
    const char* attr = RANDOM_WORDS[attr_index];
    bool is_equality_node = random_bool(); 
    if(is_equality_node) {
        enum ast_equality_e op = random_in_range(0, 1);
        int64_t integer_value = random_in_range(value_min, value_max);
        struct equality_value value = { .value_type = AST_EQUALITY_VALUE_INTEGER, .integer_value = integer_value };
        binary_node = ast_equality_expr_create(op, attr, value);
    }
    else {
        enum ast_compare_e op = random_in_range(0, 3);
        int64_t integer_value = random_in_range(value_min, value_max);
        struct compare_value value = { .value_type = AST_COMPARE_VALUE_INTEGER, .integer_value = integer_value };
        binary_node = ast_compare_expr_create(op, attr, value);
    }
    return binary_node;
}
 
const struct ast_node* generate_expr(size_t complexity, size_t attr_min, size_t attr_max, int64_t value_min, int64_t value_max)
{
    struct ast_node* last_combi_node = NULL;
    for(size_t j = 0; j < complexity; j++) {
        struct ast_node* bin_node = (struct ast_node*)make_binary_expr(attr_min, attr_max, value_min, value_max);

        enum ast_bool_e boolop = random_in_range(0, 1);
        if(j == 0) {
            struct ast_node* another_bin_node = (struct ast_node*)make_binary_expr(attr_min, attr_max, value_min, value_max);
            last_combi_node = ast_bool_expr_binary_create(boolop, bin_node, another_bin_node);
        }
        else {
            last_combi_node = ast_bool_expr_binary_create(boolop, bin_node, last_combi_node);
        }
    }
    return last_combi_node;
}

void write_expr(FILE* f, const struct ast_node* node)
{
    switch(node->type) {
        case(AST_TYPE_SET_EXPR): {
            fprintf(stderr, "should never happen for now");
            abort();
        }
        case(AST_TYPE_LIST_EXPR): {
            fprintf(stderr, "should never happen for now");
            abort();
        }
        case(AST_TYPE_SPECIAL_EXPR): {
            fprintf(stderr, "should never happen for now");
            abort();
        }
        case(AST_TYPE_COMPARE_EXPR): {
            fprintf(f, "%s ", node->compare_expr.attr_var.attr);
            switch(node->compare_expr.op) {
                case AST_COMPARE_LT: {
                    fprintf(f, "< ");
                    break;
                }
                case AST_COMPARE_LE: {
                    fprintf(f, "<= ");
                    break;
                }
                case AST_COMPARE_GT: {
                    fprintf(f, "> ");
                    break;
                }
                case AST_COMPARE_GE: {
                    fprintf(f, ">= ");
                    break;
                }
                default: {
                    switch_default_error("Invalid compare operation");
                }
            }
            fprintf(f, "%" PRIu64 " ", node->compare_expr.value.integer_value);
            break;
        }
        case AST_TYPE_EQUALITY_EXPR : {
            fprintf(f, "%s ", node->equality_expr.attr_var.attr);
            switch(node->equality_expr.op) {
                case AST_EQUALITY_EQ: {
                    fprintf(f, "= ");
                    break;
                }
                case AST_EQUALITY_NE: {
                    fprintf(f, "<> ");
                    break;
                }
                default: {
                    switch_default_error("Invalid equality operation");
                }
            }
            fprintf(f, "%" PRIu64 " ", node->equality_expr.value.integer_value);
            break;
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_VARIABLE: {
                    fprintf(f, "%s", node->bool_expr.variable.attr);
                    break;
                }
                case AST_BOOL_NOT: {
                    fprintf(f, "not ");
                    write_expr(f, node->bool_expr.unary.expr);
                    break;
                }
                case(AST_BOOL_OR):
                case(AST_BOOL_AND): {
                    write_expr(f, node->bool_expr.binary.lhs);
                    switch(node->bool_expr.op) {
                        case AST_BOOL_AND: {
                            fprintf(f, "&& ");
                            break;
                        }
                        case AST_BOOL_OR: {
                            fprintf(f, "|| ");
                            break;
                        }
                        case AST_BOOL_NOT:
                        case AST_BOOL_VARIABLE: 
                        default: {
                            switch_default_error("Invalid bool operation");
                        }
                    }
                    write_expr(f, node->bool_expr.binary.rhs);
                    break;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                }
            }
            break;
        }
        default: {
            switch_default_error("Invalid node type");
        }
    }
}

int main(void)
{
    unsigned int expr_count = 100;
    unsigned int attr_min = 0;
    unsigned int attr_max = 29;
    unsigned int value_min = 0;
    unsigned int value_max = 100;
    unsigned int complexity_min = 5;
    unsigned int complexity_max = 25;
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
