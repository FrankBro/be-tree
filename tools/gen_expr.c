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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "ast.h"
#include "random_words.h"
 
double mean(double* values, int n)
{
    int i;
    double s = 0;
 
    for ( i = 0; i < n; i++ )
        s += values[i];
    return s / n;
}
 
 
double stddev(double* values, int n)
{
    int i;
    double average = mean(values,n);
    double s = 0;
 
    for ( i = 0; i < n; i++ )
        s += (values[i] - average) * (values[i] - average);
    return sqrt(s / (n - 1));
}
 
/*
 * Normal random numbers generator - Marsaglia algorithm.
 */
double* generate(int n)
{
    int i;
    int m = n + n % 2;
    double* values = (double*)calloc(m,sizeof(double));
    double average, deviation;
 
    if ( values )
    {
        for ( i = 0; i < m; i += 2 )
        {
            double x,y,rsq,f;
            do {
                x = 2.0 * rand() / (double)RAND_MAX - 1.0;
                y = 2.0 * rand() / (double)RAND_MAX - 1.0;
                rsq = x * x + y * y;
            }while( rsq >= 1. || rsq == 0. );
            f = sqrt( -2.0 * log(rsq) / rsq );
            values[i]   = x * f;
            values[i+1] = y * f;
        }
    }
    return values;
}
 
 
void printHistogram(double* values, double low, double high, double delta, double count)
{
    const int width = 50;    
    int max = 0;
 
    int i,j,k;
    int nbins = (int)((high - low) / delta);
    int* bins = (int*)calloc(nbins,sizeof(int));
    if ( bins != NULL )
    {
        for ( i = 0; i < count; i++ )
        {
            int j = (int)( (values[i] - low) / delta );
            if ( 0 <= j  &&  j < nbins )
                bins[j]++;
        }
 
        for ( j = 0; j < nbins; j++ )
            if ( max < bins[j] )
                max = bins[j];
 
        for ( j = 0; j < nbins; j++ )
        {
            printf("(%5.2f, %5.2f) |", low + j * delta, low + (j + 1) * delta );
            k = (int)( (double)width * (double)bins[j] / (double)max );
            while(k-- > 0) putchar('*');
            printf("  %-.1f%%", bins[j] * 100.0 / (double)count);
            putchar('\n');
        }
 
        free(bins);
    }
}

unsigned int random_in_range(unsigned int min, unsigned int max)
{
    return rand() % (max + 1 - min) + min;
}

void debug(double* seq, unsigned int min, unsigned int max, unsigned int count)
{
    printf("mean = %g, stddev = %g\n\n", mean(seq,count), stddev(seq,count));
    const double low   = (double)min;
    const double high  = (double)max;
    const double delta = 1.;
    printHistogram(seq, low, high, delta, count);
}

const struct ast_node* generate_expr(unsigned int complexity, unsigned int attr_count, unsigned int value_min, unsigned int value_max)
{
    struct ast_node* last_combi_node;
    for(unsigned int j = 0; j < complexity; j++) {
        enum ast_binop_e binop = random_in_range(0, 5);
        unsigned int attr_index = random_in_range(0, attr_count-1);
        const char* attr = RANDOM_WORDS[attr_index];
        unsigned int value = random_in_range(value_min, value_max);
        struct ast_node* bin_node = ast_binary_expr_create(binop, attr, value);

        enum ast_combi_e combiop = random_in_range(0, 1);
        if(j == 0) {
            unsigned int another_attr_index = random_in_range(0, attr_count-1);
            const char* another_attr = RANDOM_WORDS[another_attr_index];
            enum ast_binop_e another_binop = random_in_range(0, 5);
            unsigned int another_value = random_in_range(value_min, value_max);
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
            fprintf(f, "%d ", node->binary_expr.value);
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
    unsigned int expr_count = 100;
    unsigned int attr_count = 500;
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
        const struct ast_node* node = generate_expr(complexity, attr_count, value_min, value_max);
        write_expr(f, node);
        fprintf(f, "\n");
        free_ast_node((struct ast_node*)node);
    }

    fclose(f);

    free(seq);
    return 0;
}