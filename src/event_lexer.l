%{
    #include <stdlib.h>
    #include "ast.h"
    #include "event_parser.h"
    #define SAVE_STRING {\
        yylval->string = malloc(yyleng - 1);\
        memcpy(yylval->string, yytext + 1, yyleng - 2);\
        yylval->string[yyleng - 2] = 0;\
    }
    #define SAVE_INTEGER yylval->integer_value = strtoll(yytext, NULL, 10)
    #define SAVE_FLOAT yylval->float_value = atof(yytext)
    #define TOKEN(t) (yylval->token = t)
    #define YYSTYPE ZZSTYPE
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-default"
    #pragma GCC diagnostic ignored "-Wsign-compare"
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif
%}

%option prefix="zz"
%option noyywrap
%option reentrant
%option bison-bridge

%%

[ \t\n]             ;
"-"                 return TOKEN(EVENT_MINUS);
"["                 return TOKEN(EVENT_LSQUARE);
"]"                 return TOKEN(EVENT_RSQUARE);
"{"                 return TOKEN(EVENT_LCURLY);
"}"                 return TOKEN(EVENT_RCURLY);
","                 return TOKEN(EVENT_COMMA);
":"                 return TOKEN(EVENT_COLON);
"true"              return TOKEN(EVENT_TRUE);
"false"             return TOKEN(EVENT_FALSE);
"null"              return TOKEN(EVENT_NULL);

\"advertiser\"      return TOKEN(EVENT_ADVERTISER);
\"advertiser:ip\"   return TOKEN(EVENT_ADVERTISERIP);
\"campaign\"        return TOKEN(EVENT_CAMPAIGN);
\"campaign:ip\"     return TOKEN(EVENT_CAMPAIGNIP);
\"flight\"          return TOKEN(EVENT_FLIGHT);
\"flight:ip\"       return TOKEN(EVENT_FLIGHTIP);
\"product\"         return TOKEN(EVENT_PRODUCT);
\"product:ip\"      return TOKEN(EVENT_PRODUCTIP);

\"(\\.|[^"\\])*\"   SAVE_STRING; return EVENT_STRING;
\'(\\.|[^'\\])*\'   SAVE_STRING; return EVENT_STRING;
[0-9]+              SAVE_INTEGER; return EVENT_INTEGER;
[0-9]+\.[0-9]*      SAVE_FLOAT; return EVENT_FLOAT;

.                   printf("Unknown token! (%s)\n", yytext); yyterminate();

%%

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

