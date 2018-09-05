%{
    #include <stdint.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <string.h>
    #include "ast.h"
    #include "betree.h"
    #include "parser.h"
    #include "value.h"
    struct ast_node *root;
    extern int xxlex();
    void xxerror(void *scanner, const char *s) { (void)scanner; printf("ERROR: %s\n", s); }
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-default"
    #pragma GCC diagnostic ignored "-Wshadow"
#endif
%}

// %debug
%pure-parser
%lex-param {void *scanner}
%parse-param {void *scanner}
%define api.prefix {xx}

%{
    int parse(const char *text, struct ast_node **node);
%}

%union {
    char *string;
    int64_t integer_value;
    double float_value;
    struct betree_integer_list* integer_list_value;
    struct betree_string_list* string_list_value;
    struct value value;
    struct string_value string_value;
    struct compare_value compare_value;
    struct equality_value equality_value;
    struct attr_var variable_value;
    struct set_left_value set_left_value;
    struct set_right_value set_right_value;
    struct list_value list_value;
    struct ast_node *node;
    int token;
}

%token<token> TMINUS
%token<token> TCEQ TCNE TCGT TCGE TCLT TCLE
%token<token> TLPAREN TRPAREN TCOMMA TNOTIN TIN TONEOF TNONEOF TALLOF
%token<token> TAND TOR
%token<token> TNOT
%token<token> TWITHINFREQUENCYCAP 
%token<token> TSEGMENTWITHIN TSEGMENTBEFORE 
%token<token> TGEOWITHINRADIUS 
%token<token> TCONTAINS TSTARTSWITH TENDSWITH 
%token<token> TUNDEFINED
%token<token> TTRUE TFALSE

%token<string> TSTRING TIDENTIFIER
%token<integer_value> TINTEGER
%token<float_value> TFLOAT

%type<node> expr num_comp_expr eq_expr set_expr list_expr bool_expr
%type<node> special_expr s_frequency_expr s_segment_expr s_geo_expr s_string_expr
%type<node> undefined_expr
%type<string> ident

%type<integer_value> integer
%type<float_value> float
%type<string_value> string
%type<compare_value> num_comp_value 
%type<equality_value> eq_value
%type<variable_value> variable_value
%type<set_left_value> set_left_value
%type<set_right_value> set_right_value
%type<list_value> list_value

%type<integer_list_value> integer_list_value integer_list_loop
%type<string_list_value> string_list_value string_list_loop

%left TCEQ TCNE TCGT TCGE TCLT TCLE
%left TOR
%left TAND 
%precedence TNOT

%start program

%%

program             : expr                                  { root = $1; }

ident               : TIDENTIFIER                           { $$ = $1; }

integer             : TINTEGER                              { $$ = $1; }
                    | TMINUS TINTEGER                       { $$ = - $2; }
;

float               : TFLOAT                                { $$ = $1; }
                    | TMINUS TFLOAT                         { $$ = - $2; }
;

string              : TSTRING                               { $$.string = strdup($1); $$.str = INVALID_STR; free($1); }

integer_list_value  : TLPAREN integer_list_loop TRPAREN     { $$ = $2; }

integer_list_loop   : integer                               { $$ = make_integer_list(); add_integer_list_value($1, $$); }
                    | integer_list_loop TCOMMA integer      { add_integer_list_value($3, $1); $$ = $1; }
;       

string_list_value   : TLPAREN string_list_loop TRPAREN      { $$ = $2; }

string_list_loop    : string                                { $$ = make_string_list(); add_string_list_value($1, $$); }
                    | string_list_loop TCOMMA string        { add_string_list_value($3, $1); $$ = $1; }
;       

expr                : TLPAREN expr TRPAREN                  { $$ = $2; }
                    | num_comp_expr                         { $$ = $1; }
                    | eq_expr                               { $$ = $1; }
                    | set_expr                              { $$ = $1; }
                    | list_expr                             { $$ = $1; }
                    | bool_expr                             { $$ = $1; }
                    | special_expr                          { $$ = $1; }
                    | undefined_expr                        { $$ = $1; }
;       

undefined_expr      : TUNDEFINED ident                      { $$ = ast_undefined_expr_create($2); }

num_comp_value      : integer                               { $$.value_type = AST_COMPARE_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                                 { $$.value_type = AST_COMPARE_VALUE_FLOAT; $$.float_value = $1; }
;       

num_comp_expr       : ident TCGT num_comp_value             { $$ = ast_compare_expr_create(AST_COMPARE_GT, $1, $3); free($1); }
                    | ident TCGE num_comp_value             { $$ = ast_compare_expr_create(AST_COMPARE_GE, $1, $3); free($1); }
                    | ident TCLT num_comp_value             { $$ = ast_compare_expr_create(AST_COMPARE_LT, $1, $3); free($1); }
                    | ident TCLE num_comp_value             { $$ = ast_compare_expr_create(AST_COMPARE_LE, $1, $3); free($1); }
                    | num_comp_value TCLT ident             { $$ = ast_compare_expr_create(AST_COMPARE_GT, $3, $1); free($3); }
                    | num_comp_value TCLE ident             { $$ = ast_compare_expr_create(AST_COMPARE_GE, $3, $1); free($3); }
                    | num_comp_value TCGT ident             { $$ = ast_compare_expr_create(AST_COMPARE_LT, $3, $1); free($3); }
                    | num_comp_value TCGE ident             { $$ = ast_compare_expr_create(AST_COMPARE_LE, $3, $1); free($3); }
;       

eq_value            : integer                               { $$.value_type = AST_EQUALITY_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                                 { $$.value_type = AST_EQUALITY_VALUE_FLOAT; $$.float_value = $1; }
                    | string                                { $$.value_type = AST_EQUALITY_VALUE_STRING; $$.string_value = $1; }
;       

eq_expr             : ident TCEQ eq_value                   { $$ = ast_equality_expr_create(AST_EQUALITY_EQ, $1, $3); free($1); }
                    | ident TCNE eq_value                   { $$ = ast_equality_expr_create(AST_EQUALITY_NE, $1, $3); free($1); }
                    | eq_value TCEQ ident                   { $$ = ast_equality_expr_create(AST_EQUALITY_EQ, $3, $1); free($3); }
                    | eq_value TCNE ident                   { $$ = ast_equality_expr_create(AST_EQUALITY_NE, $3, $1); free($3); }
;       

variable_value      : ident                                 { $$ = make_attr_var($1, NULL); free($1); }

set_left_value      : integer                               { $$.value_type = AST_SET_LEFT_VALUE_INTEGER; $$.integer_value = $1; }
                    | string                                { $$.value_type = AST_SET_LEFT_VALUE_STRING; $$.string_value = $1; }
                    | variable_value                        { $$.value_type = AST_SET_LEFT_VALUE_VARIABLE; $$.variable_value = $1; }
;       

set_right_value     : integer_list_value                    { $$.value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST; $$.integer_list_value = $1; }
                    | string_list_value                     { $$.value_type = AST_SET_RIGHT_VALUE_STRING_LIST; $$.string_list_value = $1; }
                    | variable_value                        { $$.value_type = AST_SET_RIGHT_VALUE_VARIABLE; $$.variable_value = $1; }
;

set_expr            : set_left_value TNOTIN set_right_value { $$ = ast_set_expr_create(AST_SET_NOT_IN, $1, $3); }
                    | set_left_value TIN set_right_value    { $$ = ast_set_expr_create(AST_SET_IN, $1, $3); }
;

list_value          : integer_list_value                    { $$.value_type = AST_LIST_VALUE_INTEGER_LIST; $$.integer_list_value = $1; }
                    | string_list_value                     { $$.value_type = AST_LIST_VALUE_STRING_LIST; $$.string_list_value = $1; }
;

list_expr           : ident TONEOF list_value               { $$ = ast_list_expr_create(AST_LIST_ONE_OF, $1, $3); free($1);}
                    | ident TNONEOF list_value              { $$ = ast_list_expr_create(AST_LIST_NONE_OF, $1, $3); free($1);}
                    | ident TALLOF list_value               { $$ = ast_list_expr_create(AST_LIST_ALL_OF, $1, $3); free($1);}
;

bool_expr           : expr TAND expr                        { $$ = ast_bool_expr_binary_create(AST_BOOL_AND, $1, $3); }
                    | expr TOR expr                         { $$ = ast_bool_expr_binary_create(AST_BOOL_OR, $1, $3); }
                    | TNOT expr                             { $$ = ast_bool_expr_unary_create($2); }
                    | ident                                 { $$ = ast_bool_expr_variable_create($1); free($1); }
                    | TTRUE                                 { $$ = ast_bool_expr_literal_create(true); }
                    | TFALSE                                { $$ = ast_bool_expr_literal_create(false); }
;                       

special_expr        : s_frequency_expr                      { $$ = $1; }
                    | s_segment_expr                        { $$ = $1; }
                    | s_geo_expr                            { $$ = $1; }
                    | s_string_expr                         { $$ = $1; }
;

s_frequency_expr    : TWITHINFREQUENCYCAP TLPAREN TSTRING TCOMMA string TCOMMA integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_frequency_create(AST_SPECIAL_WITHINFREQUENCYCAP, $3, $5, $7, $9); free($3); }
;

s_segment_expr      : TSEGMENTWITHIN TLPAREN integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_segment_create(AST_SPECIAL_SEGMENTWITHIN, NULL, $3, $5); }
                    | TSEGMENTWITHIN TLPAREN ident TCOMMA integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_segment_create(AST_SPECIAL_SEGMENTWITHIN, $3, $5, $7); free($3); }
                    | TSEGMENTBEFORE TLPAREN integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_segment_create(AST_SPECIAL_SEGMENTBEFORE, NULL, $3, $5); }
                    | TSEGMENTBEFORE TLPAREN ident TCOMMA integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_segment_create(AST_SPECIAL_SEGMENTBEFORE, $3, $5, $7); free($3); }
;

s_geo_expr          : TGEOWITHINRADIUS TLPAREN integer TCOMMA integer TCOMMA integer TRPAREN
                                                            { $$ = ast_special_geo_create(AST_SPECIAL_GEOWITHINRADIUS, (double)$3, (double)$5, true, (double)$7); }
                    | TGEOWITHINRADIUS TLPAREN float TCOMMA float TCOMMA float TRPAREN
                                                            { $$ = ast_special_geo_create(AST_SPECIAL_GEOWITHINRADIUS, $3, $5, true, $7); }
;

s_string_expr       : TCONTAINS TLPAREN ident TCOMMA string TRPAREN
                                                            { $$ = ast_special_string_create(AST_SPECIAL_CONTAINS, $3, $5.string); free($3); free((char*)$5.string); }
                    | TSTARTSWITH TLPAREN ident TCOMMA string TRPAREN
                                                            { $$ = ast_special_string_create(AST_SPECIAL_STARTSWITH, $3, $5.string); free($3); free((char*)$5.string); }
                    | TENDSWITH TLPAREN ident TCOMMA string TRPAREN
                                                            { $$ = ast_special_string_create(AST_SPECIAL_ENDSWITH, $3, $5.string); free($3); free((char*)$5.string); }
;

%%

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "lexer.h"

int parse(const char *text, struct ast_node **node)
{
    // xxdebug = 1;
    
    // Parse using Bison.
    yyscan_t scanner;
    xxlex_init(&scanner);
    YY_BUFFER_STATE buffer = xx_scan_string(text, scanner);
    int rc = xxparse(scanner);
    xx_delete_buffer(buffer, scanner);
    xxlex_destroy(scanner);
    
    if(rc == 0) {
        *node = root;
        return 0;
    }
    else {
        return -1;
    }
}
