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
    struct value value;
    struct ast_node *node;
    int token;
}

%token<token> TCEQ TCNE TCGT TCGE TCLT TCLE
%token<token> TLPAREN TRPAREN
%token<token> TAND TOR
%token<token> TNOT
%token<string> TIDENTIFIER
%token<boolean_value> TTRUE TFALSE
%token<integer_value> TINTEGER
%token<float_value> TFLOAT

%type<node> expr bexpr_i bexpr_f cexpr boolexpr
%type<string> ident
%type<value> boolean
%type<value> integer
%type<value> float

%left TCEQ TCNE TCGT TCGE TCLT TCLE
%left TAND TOR
%left TNOT

%start program

%%

program     : expr                  { root = $1; }

ident       : TIDENTIFIER           { $$ = $1; }

boolean     : TTRUE                 { $$.value_type = VALUE_B; $$.bvalue = true; }
            | TFALSE                { $$.value_type = VALUE_B; $$.bvalue = false; }

integer     : TINTEGER              { $$.value_type = VALUE_I; $$.ivalue = $1; }

float       : TFLOAT                { $$.value_type = VALUE_F; $$.fvalue = $1; }

expr        : TLPAREN expr TRPAREN  { $$ = $2; }
            | bexpr_i               { $$ = $1; }
            | bexpr_f               { $$ = $1; }
            | cexpr                 { $$ = $1; }
            | boolexpr              { $$ = $1; }
;

bexpr_i     : ident TCEQ integer    { $$ = ast_binary_expr_create(AST_BINOP_EQ, $1, $3); free($1); }
            | ident TCNE integer    { $$ = ast_binary_expr_create(AST_BINOP_NE, $1, $3); free($1); }
            | ident TCGT integer    { $$ = ast_binary_expr_create(AST_BINOP_GT, $1, $3); free($1); }
            | ident TCGE integer    { $$ = ast_binary_expr_create(AST_BINOP_GE, $1, $3); free($1); }
            | ident TCLT integer    { $$ = ast_binary_expr_create(AST_BINOP_LT, $1, $3); free($1); }
            | ident TCLE integer    { $$ = ast_binary_expr_create(AST_BINOP_LE, $1, $3); free($1); }
;

bexpr_f     : ident TCEQ float      { $$ = ast_binary_expr_create(AST_BINOP_EQ, $1, $3); free($1); }
            | ident TCNE float      { $$ = ast_binary_expr_create(AST_BINOP_NE, $1, $3); free($1); }
            | ident TCGT float      { $$ = ast_binary_expr_create(AST_BINOP_GT, $1, $3); free($1); }
            | ident TCGE float      { $$ = ast_binary_expr_create(AST_BINOP_GE, $1, $3); free($1); }
            | ident TCLT float      { $$ = ast_binary_expr_create(AST_BINOP_LT, $1, $3); free($1); }
            | ident TCLE float      { $$ = ast_binary_expr_create(AST_BINOP_LE, $1, $3); free($1); }
;

cexpr       : expr TAND expr        { $$ = ast_combi_expr_create(AST_COMBI_AND, $1, $3); }
            | expr TOR expr         { $$ = ast_combi_expr_create(AST_COMBI_OR, $1, $3); }
;

boolexpr    : TNOT ident            { $$ = ast_bool_expr_create(AST_BOOL_NOT, $2); free($2); }
            | ident                 { $$ = ast_bool_expr_create(AST_BOOL_NONE, $1); free($1); }
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