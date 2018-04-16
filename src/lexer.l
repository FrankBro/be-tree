%{
    #include "ast.h"
    #include "parser.h"
    #define SAVE_STRING yylval->string = strndup(yytext, yyleng)
    #define SAVE_INTEGER yylval->integer = atoi(yytext)
    #define SAVE_FLOAT yylval->float_value = atof(yytext)
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
"="                     return TOKEN(TCEQ);
"<>"                    return TOKEN(TCNE);
">"                     return TOKEN(TCGT);
">="                    return TOKEN(TCGE);
"<"                     return TOKEN(TCLT);
"<="                    return TOKEN(TCLE);
"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"&&"                    return TOKEN(TAND);
"and"                   return TOKEN(TAND);
"||"                    return TOKEN(TOR);
"or"                    return TOKEN(TOR);
[a-zA-Z_][a-zA-Z0-9_-]* SAVE_STRING; return TIDENTIFIER;
[0-9]*                  SAVE_INTEGER; return TINTEGER;
[0-9]+\.[0-9]*          SAVE_FLOAT; return TFLOAT;
.                       printf("Unknown token! (%s)\n", yytext); yyterminate();

%%

#pragma clang diagnostic pop
