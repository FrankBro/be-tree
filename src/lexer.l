%{
    #include "ast.h"
    #include "parser.h"
    #define SAVE_STRING yylval->string = strndup(yytext, yyleng)
    #define SAVE_INTEGER yylval->integer = atoi(yytext)
    #define TOKEN(t) (yylval->token = t)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wunused-function"
%}

%option noyywrap
%option reentrant
%option bison-bridge

%%

[ \t\n]                 ;
[a-zA-Z_][a-zA-Z0-9_]*  SAVE_STRING; return TIDENTIFIER;
[0-9]*                  SAVE_INTEGER; return TINTEGER;
"="                     return TOKEN(TCEQ);
"<>"                    return TOKEN(TCNE);
">"                     return TOKEN(TCGT);
">="                    return TOKEN(TCGE);
"<"                     return TOKEN(TCLT);
"<="                    return TOKEN(TCLE);
"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"&&"                    return TOKEN(TAND);
"||"                    return TOKEN(TOR);
.                       printf("Unknown token!\n"); yyterminate();

%%

#pragma clang diagnostic pop
