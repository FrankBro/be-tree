%{
    #include "stdio.h"
    #include "ast.h"
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
    int integer;
    struct ast_node *node;
    int token;
}

%token<string> TIDENTIFIER
%token<integer> TINTEGER
%token<token> TCEQ TCNE TCGT TCGE TCLT TCLE
%token<token> TLPAREN TRPAREN
%token<token> TAND TOR

%type<node> expr bexpr cexpr
%type<string> ident
%type<integer> integer

%left TCEQ TCNE TCGT TCGE TCLT TCLE
%left TAND TOR

%start program

%%

program : expr                  { root = $1; }

ident   : TIDENTIFIER           { $$ = $1; }

integer : TINTEGER              { $$ = $1; }

expr    : TLPAREN expr TRPAREN  { $$ = $2; }
        | bexpr                 { $$ = $1; }
        | cexpr                 { $$ = $1; }
;

bexpr   : ident TCEQ integer    { $$ = ast_binary_expr_create(BINOP_EQ, $1, $3); free($1); }
        | ident TCNE integer    { $$ = ast_binary_expr_create(BINOP_NE, $1, $3); free($1); }
        | ident TCGT integer    { $$ = ast_binary_expr_create(BINOP_GT, $1, $3); free($1); }
        | ident TCGE integer    { $$ = ast_binary_expr_create(BINOP_GE, $1, $3); free($1); }
        | ident TCLT integer    { $$ = ast_binary_expr_create(BINOP_LT, $1, $3); free($1); }
        | ident TCLE integer    { $$ = ast_binary_expr_create(BINOP_LE, $1, $3); free($1); }
;

cexpr   : expr TAND expr        { $$ = ast_combi_expr_create(COMBI_AND, $1, $3); }
        | expr TOR expr         { $$ = ast_combi_expr_create(COMBI_OR, $1, $3); }
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