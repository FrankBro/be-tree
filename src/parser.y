%{
    #include <stdint.h>
    #include <stdbool.h>
    #include "stdio.h"
    #include "ast.h"
    #include "betree.h"
    #include "parser.h"
    #include "lexer.h"
    struct ast_node *root;
    extern int yylex();
    void yyerror(void *scanner, const char *s) { (void)scanner; printf("ERROR: %s\n", s); }
%}

// %debug
%pure-parser
%lex-param {void *scanner}
%parse-param {void *scanner}

%{
    int parse(const char *text, struct ast_node **node);
%}

%union {
    char *string;
    bool boolean_value;
    int64_t integer_value;
    double float_value;
    struct integer_list_value integer_list_value;
    struct string_list_value string_list_value;
    struct value value;
    struct string_value string_value;
    struct numeric_compare_value numeric_compare_value;
    struct equality_value equality_value;
    struct variable_value variable_value;
    struct set_left_value set_left_value;
    struct set_right_value set_right_value;
    struct list_value list_value;
    struct ast_node *node;
    int token;
}

%token<token> TCEQ TCNE TCGT TCGE TCLT TCLE
%token<token> TLPAREN TRPAREN TCOMMA TNOTIN TIN TONEOF TNONEOF TALLOF
%token<token> TAND TOR
%token<token> TNOT
%token<string> TSTRING TIDENTIFIER
%token<boolean_value> TTRUE TFALSE
%token<integer_value> TINTEGER
%token<float_value> TFLOAT

%type<node> expr num_comp_expr eq_expr set_expr list_expr cexpr boolexpr
%type<string> ident
%type<value> boolean

%type<integer_value> integer
%type<float_value> float
%type<string_value> string
%type<numeric_compare_value> num_comp_value 
%type<equality_value> eq_value
%type<variable_value> variable_value
%type<set_left_value> set_left_value
%type<set_right_value> set_right_value
%type<list_value> list_value

%type<integer_list_value> integer_list_value integer_list_loop
%type<string_list_value> string_list_value string_list_loop

%left TCEQ TCNE TCGT TCGE TCLT TCLE
%left TAND TOR
%left TNOT

%start program

%%

program             : expr                                  { root = $1; }

ident               : TIDENTIFIER                           { $$ = $1; }

boolean             : TTRUE                                 { $$.value_type = VALUE_B; $$.bvalue = true; }
                    | TFALSE                                { $$.value_type = VALUE_B; $$.bvalue = false; }

integer             : TINTEGER                              { $$ = $1; }

float               : TFLOAT                                { $$ = $1; }

string              : TSTRING                               { $$.string = strdup($1); $$.str = -1; free($1); }

integer_list_value  : TLPAREN integer_list_loop TRPAREN     { $$ = $2; }

integer_list_loop   : TINTEGER                              { $$.count = 0; $$.integers = NULL; add_integer_list_value($1, &$$); }
                    | integer_list_loop TCOMMA TINTEGER     { add_integer_list_value($3, &$1); $$ = $1; }
;       

string_list_value   : TLPAREN string_list_loop TRPAREN      { $$ = $2; }

string_list_loop    : string                                { $$.count = 0; $$.strings = NULL; add_string_list_value($1, &$$); }
                    | string_list_loop TCOMMA string        { add_string_list_value($3, &$1); $$ = $1; }
;       

expr                : TLPAREN expr TRPAREN                  { $$ = $2; }
                    | num_comp_expr                         { $$ = $1; }
                    | eq_expr                               { $$ = $1; }
                    | set_expr                              { $$ = $1; }
                    | list_expr                             { $$ = $1; }
                    | cexpr                                 { $$ = $1; }
                    | boolexpr                              { $$ = $1; }
;       

num_comp_value      : integer                               { $$.value_type = AST_NUMERIC_COMPARE_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                                 { $$.value_type = AST_NUMERIC_COMPARE_VALUE_FLOAT; $$.float_value = $1; }
;       

num_comp_expr       : ident TCGT num_comp_value             { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GT, $1, $3); free($1); }
                    | ident TCGE num_comp_value             { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, $1, $3); free($1); }
                    | ident TCLT num_comp_value             { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LT, $1, $3); free($1); }
                    | ident TCLE num_comp_value             { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LE, $1, $3); free($1); }
;       

eq_value            : integer                               { $$.value_type = AST_EQUALITY_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                                 { $$.value_type = AST_EQUALITY_VALUE_FLOAT; $$.integer_value = $1; }
                    | string                                { $$.value_type = AST_EQUALITY_VALUE_STRING; $$.string_value = $1; }
;       

eq_expr             : ident TCEQ eq_value                   { $$ = ast_equality_expr_create(AST_EQUALITY_EQ, $1, $3); free($1); }
                    | ident TCNE eq_value                   { $$ = ast_equality_expr_create(AST_EQUALITY_NE, $1, $3); free($1); }
;       

variable_value      : ident                                 { $$.name = strdup($1); $$.variable_id = -1; free($1); }

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

cexpr               : expr TAND expr                        { $$ = ast_combi_expr_create(AST_COMBI_AND, $1, $3); }
                    | expr TOR expr                         { $$ = ast_combi_expr_create(AST_COMBI_OR, $1, $3); }
;                       

boolexpr            : TNOT ident                            { $$ = ast_bool_expr_create(AST_BOOL_NOT, $2); free($2); }
                    | ident                                 { $$ = ast_bool_expr_create(AST_BOOL_NONE, $1); free($1); }
;

%%

int parse(const char *text, struct ast_node **node)
{
    // yydebug = 1;
    
    // Parse using Bison.
    yyscan_t scanner;
    yylex_init(&scanner);
    YY_BUFFER_STATE buffer = yy_scan_string(text, scanner);
    int rc = yyparse(scanner);
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);
    
    if(rc == 0) {
        *node = root;
        return 0;
    }
    else {
        return -1;
    }
}