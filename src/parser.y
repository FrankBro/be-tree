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

//%debug
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
    struct integer_list integer_list_value;
    struct value value;
    struct string_value string_value;
    struct numeric_compare_value numeric_compare_value;
    struct equality_value equality_value;
    struct ast_node *node;
    int token;
}

%token<token> TCEQ TCNE TCGT TCGE TCLT TCLE
%token<token> TLPAREN TRPAREN TCOMMA TNOTIN TIN
%token<token> TAND TOR
%token<token> TNOT
%token<token> TQUOTE
%token<string> TIDENTIFIER
%token<boolean_value> TTRUE TFALSE
%token<integer_value> TINTEGER
%token<float_value> TFLOAT

%type<node> expr ncexpr eexpr bexpr_il cexpr boolexpr
%type<string> ident
%type<value> boolean
%type<value> integer_list

%type<integer_value> integer
%type<float_value> float
%type<string_value> string
%type<numeric_compare_value> ncvalue 
%type<equality_value> evalue

%type<integer_list_value> integer_list_loop

%left TCEQ TCNE TCGT TCGE TCLT TCLE
%left TAND TOR
%left TNOT

%start program

%%

program             : expr                              { root = $1; }

ident               : TIDENTIFIER                       { $$ = $1; }

boolean             : TTRUE                             { $$.value_type = VALUE_B; $$.bvalue = true; }
                    | TFALSE                            { $$.value_type = VALUE_B; $$.bvalue = false; }

integer             : TINTEGER                          { $$ = $1; }

float               : TFLOAT                            { $$ = $1; }

string              : TQUOTE ident TQUOTE               { $$.string = strdup($2); $$.str = -1; free($2); }

integer_list        : TLPAREN integer_list_loop TRPAREN { $$.value_type = VALUE_IL; $$.ilvalue = $2; }

integer_list_loop   : TINTEGER                          { $$.count = 0; $$.integers = NULL; add_integer_list($1, &$$); }
                    | integer_list_loop TCOMMA TINTEGER { add_integer_list($3, &$1); $$ = $1; }
;

expr                : TLPAREN expr TRPAREN              { $$ = $2; }
                    | ncexpr                            { $$ = $1; }
                    | eexpr                             { $$ = $1; }
                    | bexpr_il                          { $$ = $1; }
                    | cexpr                             { $$ = $1; }
                    | boolexpr                          { $$ = $1; }
;

ncvalue             : integer                           { $$.value_type = AST_NUMERIC_COMPARE_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                             { $$.value_type = AST_NUMERIC_COMPARE_VALUE_FLOAT; $$.float_value = $1; }
;

ncexpr              : ident TCGT ncvalue                { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GT, $1, $3); free($1); }
                    | ident TCGE ncvalue                { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_GE, $1, $3); free($1); }
                    | ident TCLT ncvalue                { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LT, $1, $3); free($1); }
                    | ident TCLE ncvalue                { $$ = ast_numeric_compare_expr_create(AST_NUMERIC_COMPARE_LE, $1, $3); free($1); }
;

evalue              : integer                           { $$.value_type = AST_EQUALITY_VALUE_INTEGER; $$.integer_value = $1; }
                    | float                             { $$.value_type = AST_EQUALITY_VALUE_FLOAT; $$.integer_value = $1; }
                    | string                            { $$.value_type = AST_EQUALITY_VALUE_STRING; $$.string_value = $1; }
;

eexpr               : ident TCEQ evalue                 { $$ = ast_equality_expr_create(AST_EQUALITY_EQ, $1, $3); free($1); }
                    | ident TCNE evalue                 { $$ = ast_equality_expr_create(AST_EQUALITY_NE, $1, $3); free($1); }
;

bexpr_il            : ident TNOTIN integer_list         { $$ = ast_list_expr_create(AST_LISTOP_NOTIN, $1, $3.ilvalue); free($1); }
                    | ident TIN integer_list            { $$ = ast_list_expr_create(AST_LISTOP_IN, $1, $3.ilvalue); free($1); }
;

cexpr               : expr TAND expr                    { $$ = ast_combi_expr_create(AST_COMBI_AND, $1, $3); }
                    | expr TOR expr                     { $$ = ast_combi_expr_create(AST_COMBI_OR, $1, $3); }
;               

boolexpr            : TNOT ident                        { $$ = ast_bool_expr_create(AST_BOOL_NOT, $2); free($2); }
                    | ident                             { $$ = ast_bool_expr_create(AST_BOOL_NONE, $1); free($1); }
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