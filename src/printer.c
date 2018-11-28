#include <float.h>
#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "ast.h"
#include "printer.h"
#include "tree.h"
#include "utils.h"

static const char* compare_value_to_string(struct compare_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_COMPARE_VALUE_INTEGER:
            if(basprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_COMPARE_VALUE_FLOAT:
            if(basprintf(&expr, "%.2f", value.float_value) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid compare value type");
            break;
    }
    return expr;
}

static const char* compare_op_to_string(enum ast_compare_e op)
{
    switch(op) {
        case AST_COMPARE_LT:
            return "<";
        case AST_COMPARE_LE:
            return "<=";
        case AST_COMPARE_GT:
            return ">";
        case AST_COMPARE_GE:
            return ">=";
        default:
            switch_default_error("Invalid compare operation");
            return NULL;
    }
}

static const char* equality_value_to_string(struct equality_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_EQUALITY_VALUE_INTEGER:
            if(basprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_EQUALITY_VALUE_FLOAT:
            if(basprintf(&expr, "%.2f", value.float_value) < 0) {
                abort();
            }
            break;
        case AST_EQUALITY_VALUE_STRING:
            if(basprintf(&expr, "\"%s\"", value.string_value.string) < 0) {
                abort();
            }
            break;
        case AST_EQUALITY_VALUE_INTEGER_ENUM:
            if(basprintf(&expr, "%ld", value.integer_enum_value.integer) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid equality value type");
            break;
    }
    return expr;
}

static const char* equality_op_to_string(enum ast_equality_e op)
{
    switch(op) {
        case AST_EQUALITY_EQ:
            return "=";
        case AST_EQUALITY_NE:
            return "<>";
        default:
            switch_default_error("Invalid equality operation");
            return NULL;
    }
}

static const char* set_left_value_to_string(struct set_left_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_LEFT_VALUE_INTEGER:
            if(basprintf(&expr, "%ld", value.integer_value) < 0) {
                abort();
            }
            break;
        case AST_SET_LEFT_VALUE_STRING:
            if(basprintf(&expr, "\"%s\"", value.string_value.string) < 0) {
                abort();
            }
            break;
        case AST_SET_LEFT_VALUE_VARIABLE:
            if(basprintf(&expr, "%s", value.variable_value.attr) < 0) {
                abort();
            }
            break;
        default:
            switch_default_error("Invalid set left value type");
            break;
    }
    return expr;
}

static const char* set_right_value_to_string(struct set_right_value value)
{
    char* expr;
    switch(value.value_type) {
        case AST_SET_RIGHT_VALUE_VARIABLE:
            if(basprintf(&expr, "%s", value.variable_value.attr) < 0) {
                abort();
            }
            break;
        case AST_SET_RIGHT_VALUE_INTEGER_LIST: {
            const char* list = integer_list_value_to_string(value.integer_list_value);
            if(basprintf(&expr, "(%s)", list) < 0) {
                abort();
            }
            bfree((char*)list);
            break;
        }
        case AST_SET_RIGHT_VALUE_STRING_LIST: {
            const char* list = string_list_value_to_string(value.string_list_value);
            if(basprintf(&expr, "(%s)", list) < 0) {
                abort();
            }
            bfree((char*)list);
            break;
        }
        case AST_SET_RIGHT_VALUE_INTEGER_LIST_ENUM: {
            const char* list = integer_list_enum_value_to_string(value.integer_list_enum_value);
            if(basprintf(&expr, "(%s)", list) < 0) {
                abort();
            }
            bfree((char*)list);
            break;
        }
        default:
            switch_default_error("Invalid set right value type");
            break;
    }
    return expr;
}

static const char* set_op_to_string(enum ast_set_e op)
{
    switch(op) {
        case AST_SET_IN:
            return "in";
        case AST_SET_NOT_IN:
            return "not in";
        default:
            switch_default_error("Invalid set op");
            return NULL;
    }
}

static const char* list_value_to_string(struct list_value value)
{
    char* list;
    switch(value.value_type) {
        case AST_LIST_VALUE_INTEGER_LIST: {
            const char* inner = integer_list_value_to_string(value.integer_list_value);
            if(basprintf(&list, "(%s)", inner) < 0) {
                abort();
            }
            bfree((char*)inner);
            break;
        }
        case AST_LIST_VALUE_STRING_LIST: {
            const char* inner = string_list_value_to_string(value.string_list_value);
            if(basprintf(&list, "(%s)", inner) < 0) {
                abort();
            }
            bfree((char*)inner);
            break;
        }
        default: {
            switch_default_error("Invalid list value type");
            break;
        }
    }
    return list;
}

static const char* list_op_to_string(enum ast_list_e op)
{
    switch(op) {
        case AST_LIST_ONE_OF: {
            return "one of";
        }
        case AST_LIST_NONE_OF: {
            return "none of";
        }
        case AST_LIST_ALL_OF: {
            return "all of";
        }
        default: {
            switch_default_error("Invalid list operation");
            return NULL;
        }
    }
}

char* ast_to_string(const struct ast_node* node)
{
    char* expr;
    switch(node->type) {
        case AST_TYPE_IS_NULL_EXPR:
            if(basprintf(&expr, "?%s", node->is_null_expr.attr_var.attr) < 0) {
                abort();
            }
            return expr;
        case(AST_TYPE_SPECIAL_EXPR): {
            switch(node->special_expr.type) {
                case AST_SPECIAL_FREQUENCY:
                    if(basprintf(&expr,
                           "within_frequency_cap(\"%s\", \"%s\", %ld, %zu)",
                           frequency_type_to_string(node->special_expr.frequency.type),
                           node->special_expr.frequency.ns.string,
                           node->special_expr.frequency.value,
                           node->special_expr.frequency.length)
                        < 0) {
                        abort();
                    }
                    return expr;
                case AST_SPECIAL_SEGMENT: {
                    const char* op;
                    switch(node->special_expr.segment.op) {
                        case AST_SPECIAL_SEGMENTWITHIN:
                            op = "segment_within";
                            break;
                        case AST_SPECIAL_SEGMENTBEFORE:
                            op = "segment_before";
                            break;
                        default:
                            switch_default_error("Invalid special segment op");
                            return NULL;
                    }
                    if(basprintf(&expr,
                           "%s(%s, %lu, %ld)",
                           op,
                           node->special_expr.segment.attr_var.attr,
                           node->special_expr.segment.segment_id,
                           node->special_expr.segment.seconds)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                case AST_SPECIAL_GEO: {
                    if(basprintf(&expr,
                           "geo_within_radius(%.2f, %.2f, %.2f)",
                           node->special_expr.geo.latitude,
                           node->special_expr.geo.longitude,
                           node->special_expr.geo.radius)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                case AST_SPECIAL_STRING: {
                    const char* op;
                    switch(node->special_expr.string.op) {
                        case AST_SPECIAL_CONTAINS:
                            op = "contains";
                            break;
                        case AST_SPECIAL_STARTSWITH:
                            op = "starts_with";
                            break;
                        case AST_SPECIAL_ENDSWITH:
                            op = "ends_with";
                            break;
                        default:
                            switch_default_error("Invalid special string op");
                            return NULL;
                    }
                    if(basprintf(&expr,
                           "%s(%s, \"%s\")",
                           op,
                           node->special_expr.string.attr_var.attr,
                           node->special_expr.string.pattern)
                        < 0) {
                        abort();
                    }
                    return expr;
                }
                default:
                    switch_default_error("Invalid special op");
                    return NULL;
            }
        }
        case(AST_TYPE_BOOL_EXPR): {
            switch(node->bool_expr.op) {
                case AST_BOOL_LITERAL: {
                    if(node->bool_expr.literal == true) {
                        return bstrdup("true");
                    }
                    else {
                        return bstrdup("false");
                    }
                }
                case AST_BOOL_VARIABLE: {
                    return bstrdup(node->bool_expr.variable.attr);
                }
                case AST_BOOL_NOT: {
                    const char* a = ast_to_string(node->bool_expr.unary.expr);
                    if(basprintf(&expr, "(not (%s))", a) < 0) {
                        abort();
                    }
                    bfree((char*)a);
                    return expr;
                }
                case AST_BOOL_OR: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    if(basprintf(&expr, "((%s) or (%s))", a, b) < 0) {
                        abort();
                    }
                    bfree((char*)a);
                    bfree((char*)b);
                    return expr;
                }
                case AST_BOOL_AND: {
                    const char* a = ast_to_string(node->bool_expr.binary.lhs);
                    const char* b = ast_to_string(node->bool_expr.binary.rhs);
                    if(basprintf(&expr, "((%s) and (%s))", a, b) < 0) {
                        abort();
                    }
                    bfree((char*)a);
                    bfree((char*)b);
                    return expr;
                }
                default: {
                    switch_default_error("Invalid bool operation");
                    return NULL;
                }
            }
        }
        case(AST_TYPE_SET_EXPR): {
            const char* left = set_left_value_to_string(node->set_expr.left_value);
            const char* right = set_right_value_to_string(node->set_expr.right_value);
            const char* op = set_op_to_string(node->set_expr.op);
            if(basprintf(&expr, "%s %s %s", left, op, right) < 0) {
                abort();
            }
            bfree((char*)left);
            bfree((char*)right);
            return expr;
        }
        case(AST_TYPE_LIST_EXPR): {
            const char* value = list_value_to_string(node->list_expr.value);
            const char* op = list_op_to_string(node->list_expr.op);
            if(basprintf(&expr, "%s %s %s", node->list_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            bfree((char*)value);
            return expr;
        }
        case(AST_TYPE_EQUALITY_EXPR): {
            const char* value = equality_value_to_string(node->equality_expr.value);
            const char* op = equality_op_to_string(node->equality_expr.op);
            if(basprintf(&expr, "%s %s %s", node->equality_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            bfree((char*)value);
            return expr;
        }
        case(AST_TYPE_COMPARE_EXPR): {
            const char* value = compare_value_to_string(node->compare_expr.value);
            const char* op = compare_op_to_string(node->compare_expr.op);
            if(basprintf(&expr, "%s %s %s", node->compare_expr.attr_var.attr, op, value) < 0) {
                abort();
            }
            bfree((char*)value);
            return expr;
        }
        default: {
            switch_default_error("Invalid expr type");
            return NULL;
        }
    }
}

static const char* value_type_to_string(enum betree_value_type_e e)
{
    switch(e) {
        case BETREE_BOOLEAN: return "boolean";
        case BETREE_INTEGER: return "integer";
        case BETREE_FLOAT: return "float";
        case BETREE_STRING: return "string";
        case BETREE_INTEGER_LIST: return "integer_list";
        case BETREE_STRING_LIST: return "string_list";
        case BETREE_SEGMENTS: return "segments";
        case BETREE_FREQUENCY_CAPS: return "frequency_caps";
        case BETREE_INTEGER_ENUM: return "integer_enum";
        case BETREE_INTEGER_LIST_ENUM: return "integer_list_enum";
        default: return "INVALID";
    }
}

void print_variable(const struct betree_variable* v) 
{
    if(v == NULL) {
        printf("undefined\n");
        return;
    }
    const char* type = value_type_to_string(v->value.value_type);
    printf("%s %s = ", type, v->attr_var.attr);
    char* inner = NULL;
    switch(v->value.value_type) {
        case BETREE_BOOLEAN:
            printf("%s", v->value.bvalue ? "true" : "false");
            break;
        case BETREE_INTEGER:
            printf("%ld", v->value.ivalue);
            break;
        case BETREE_FLOAT:
            printf("%.2f", v->value.fvalue);
            break;
        case BETREE_STRING:
            printf("%s", v->value.svalue.string);
            break;
        case BETREE_INTEGER_LIST:
            inner = integer_list_value_to_string(v->value.ilvalue);
            printf("%s", inner);
            break;
        case BETREE_STRING_LIST:
            inner = string_list_value_to_string(v->value.slvalue);
            printf("%s", inner);
            break;
        case BETREE_INTEGER_ENUM:
            printf("%ld", v->value.ievalue.integer);
            break;
        case BETREE_INTEGER_LIST_ENUM:
            inner = integer_list_enum_value_to_string(v->value.ilevalue);
            printf("%s", inner);
            break;
        case BETREE_SEGMENTS:
            inner = segments_value_to_string(v->value.segments_value);
            printf("%s", inner);
            break;
        case BETREE_FREQUENCY_CAPS:
            inner = frequency_caps_value_to_string(v->value.frequency_value);
            printf("%s", inner);
            break;
        default:
            printf("INVALID");
            break;
    }
    printf("\n");
    if(inner != NULL) {
        bfree(inner);
    }
}

void print_attr_domain(const struct attr_domain* domain)
{
    switch(domain->bound.value_type) {
        case BETREE_BOOLEAN:
            printf("%s (bool%s) [%s, %s]\n", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "",
              domain->bound.bmin ? "true" : "false",
              domain->bound.bmax ? "true" : "false");
            break;
        case BETREE_INTEGER:
            printf("%s (integer%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.imin == INT64_MIN) {
                printf("INT64_MIN, ");
            }
            else {
                printf("%ld, ", domain->bound.imin);
            }
            if(domain->bound.imax == INT64_MAX) {
                printf("INT64_MAX]\n");
            }
            else {
                printf("%ld]\n", domain->bound.imax);
            }
            break;
        case BETREE_FLOAT:
            printf("%s (float%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(feq(domain->bound.fmin, -DBL_MAX)) {
                printf("-DBL_MAX, ");
            }
            else {
                printf("%.2f, ", domain->bound.fmin);
            }
            if(feq(domain->bound.fmax, DBL_MAX)) {
                printf("DBL_MAX]\n");
            }
            else {
                printf("%.2f]\n", domain->bound.fmax);
            }
            break;
        case BETREE_STRING:
            printf("%s (string%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", domain->bound.smax);
            }
            break;
        case BETREE_INTEGER_LIST:
            printf("%s (integer list%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.imin == INT64_MIN) {
                printf("INT64_MIN, ");
            }
            else {
                printf("%ld, ", domain->bound.imin);
            }
            if(domain->bound.imax == INT64_MAX) {
                printf("INT64_MAX]\n");
            }
            else {
                printf("%ld]\n", domain->bound.imax);
            }
            break;
        case BETREE_STRING_LIST:
            printf("%s (string list%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", domain->bound.smax);
            }
            break;
        case BETREE_INTEGER_ENUM:
            printf("%s (integer enum%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", domain->bound.smax);
            }
            break;
        case BETREE_INTEGER_LIST_ENUM:
            printf("%s (integer list enum%s) [", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            if(domain->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", domain->bound.smax);
            }
            break;
        case BETREE_SEGMENTS:
            printf("%s (segments%s)\n", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            break;
        case BETREE_FREQUENCY_CAPS:
            printf("%s (frequency caps%s)\n", domain->attr_var.attr,
              domain->allow_undefined ? "?" : "");
            break;
        default:
            abort();
    }
}

void print_cdir(const struct cdir* cdir)
{
    switch(cdir->bound.value_type) {
        case BETREE_BOOLEAN:
            printf("%s (bool) [%s, %s]\n", cdir->attr_var.attr,
              cdir->bound.bmin ? "true" : "false",
              cdir->bound.bmax ? "true" : "false");
            break;
        case BETREE_INTEGER:
            printf("%s (integer) [", cdir->attr_var.attr);
            if(cdir->bound.imin == INT64_MIN) {
                printf("INT64_MIN, ");
            }
            else {
                printf("%ld, ", cdir->bound.imin);
            }
            if(cdir->bound.imax == INT64_MAX) {
                printf("INT64_MAX]\n");
            }
            else {
                printf("%ld]\n", cdir->bound.imax);
            }
            break;
        case BETREE_FLOAT:
            printf("%s (float) [", cdir->attr_var.attr);
            if(feq(cdir->bound.fmin, -DBL_MAX)) {
                printf("-DBL_MAX, ");
            }
            else {
                printf("%.2f, ", cdir->bound.fmin);
            }
            if(feq(cdir->bound.fmax, DBL_MAX)) {
                printf("DBL_MAX]\n");
            }
            else {
                printf("%.2f]\n", cdir->bound.fmax);
            }
            break;
        case BETREE_STRING:
            printf("%s (string) [%zu, ", cdir->attr_var.attr, cdir->bound.smin);
            if(cdir->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", cdir->bound.smax);
            }
            break;
        case BETREE_INTEGER_LIST:
            printf("%s (integer list) [", cdir->attr_var.attr);
            if(cdir->bound.imin == INT64_MIN) {
                printf("INT64_MIN, ");
            }
            else {
                printf("%ld, ", cdir->bound.imin);
            }
            if(cdir->bound.imax == INT64_MAX) {
                printf("INT64_MAX]\n");
            }
            else {
                printf("%ld]\n", cdir->bound.imax);
            }
            break;
        case BETREE_STRING_LIST:
            printf("%s (string list) [%zu, ", cdir->attr_var.attr, cdir->bound.smin);
            if(cdir->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", cdir->bound.smax);
            }
            break;
        case BETREE_INTEGER_ENUM:
            printf("%s (integer enum) [%zu, ", cdir->attr_var.attr, cdir->bound.smin);
            if(cdir->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", cdir->bound.smax);
            }
            break;
        case BETREE_INTEGER_LIST_ENUM:
            printf("%s (integer list enum) [%zu, ", cdir->attr_var.attr, cdir->bound.smin);
            if(cdir->bound.smax >= SIZE_MAX - 1) {
                printf("SIZE_MAX]\n");
            }
            else {
                printf("%zu]\n", cdir->bound.smax);
            }
            break;
        case BETREE_SEGMENTS: abort();
        case BETREE_FREQUENCY_CAPS: abort();
        default: abort();
    }
}

void print_report(const struct report* report)
{
    fprintf(stderr, "evaluated = %zu, matched = %zu, memoized = %zu, shorted = %zu\n",
        report->evaluated, report->matched, report->memoized, report->shorted);
}

void print_value_type(enum betree_value_type_e value_type)
{
    switch(value_type) {
        case BETREE_BOOLEAN: printf("BETREE_BOOLEAN\n"); break;
        case BETREE_INTEGER: printf("BETREE_INTEGER\n"); break;
        case BETREE_FLOAT: printf("BETREE_FLOAT\n"); break;
        case BETREE_STRING: printf("BETREE_STRING\n"); break;
        case BETREE_INTEGER_LIST: printf("BETREE_INTEGER_LIST\n"); break;
        case BETREE_STRING_LIST: printf("BETREE_STRING_LIST\n"); break;
        case BETREE_SEGMENTS: printf("BETREE_SEGMENTS\n"); break;
        case BETREE_FREQUENCY_CAPS: printf("BETREE_FREQUENCY_CAPS\n"); break;
        case BETREE_INTEGER_ENUM: printf("BETREE_INTEGER_ENUM\n"); break;
        case BETREE_INTEGER_LIST_ENUM: printf("BETREE_INTEGER_LIST_ENUM\n"); break;
        default: abort();
    }
}

