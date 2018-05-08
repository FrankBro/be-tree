%{
    #include <stdlib.h>
    #include "ast.h"
    #include "parser.h"
    #define SAVE_STRING {\
        yylval->string = malloc(yyleng - 1);\
        memcpy(yylval->string, yytext + 1, yyleng - 2);\
        yylval->string[yyleng - 2] = 0;\
    }
    #define SAVE_IDENTIFIER yylval->string = strndup(yytext, yyleng)
    #define SAVE_INTEGER yylval->integer_value = strtoll(yytext, NULL, 10)
    #define SAVE_FLOAT yylval->float_value = atof(yytext)
    #define TOKEN(t) (yylval->token = t)
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

%option noyywrap
%option reentrant
%option bison-bridge

%%

[ \t\n]                     ;
"within_frequency_cap"      return TOKEN(TWITHINFREQUENCYCAP);
"segment_within"            return TOKEN(TSEGMENTWITHIN);
"segment_before"            return TOKEN(TSEGMENTBEFORE);
"geo_within_radius"         return TOKEN(TGEOWITHINRADIUS);
"contains"                  return TOKEN(TCONTAINS);
"starts_with"               return TOKEN(TSTARTSWITH);
"ends_with"                 return TOKEN(TENDSWITH);
"="                         return TOKEN(TCEQ);
"<>"                        return TOKEN(TCNE);
">"                         return TOKEN(TCGT);
">="                        return TOKEN(TCGE);
"<"                         return TOKEN(TCLT);
"<="                        return TOKEN(TCLE);
"("                         return TOKEN(TLPAREN);
")"                         return TOKEN(TRPAREN);
","                         return TOKEN(TCOMMA);
"not in"                    return TOKEN(TNOTIN);
"in"                        return TOKEN(TIN);
"one of"                    return TOKEN(TONEOF);
"none of"                   return TOKEN(TNONEOF);
"all of"                    return TOKEN(TALLOF);
"&&"                        return TOKEN(TAND);
"and"                       return TOKEN(TAND);
"||"                        return TOKEN(TOR);
"or"                        return TOKEN(TOR);
"not"                       return TOKEN(TNOT);
"true"                      return TOKEN(TTRUE);
"false"                     return TOKEN(TFALSE);
\"advertiser\"              return TOKEN(TADVERTISER);
\"advertiser:ip\"           return TOKEN(TADVERTISERIP);
\"campaign\"                return TOKEN(TCAMPAIGN);
\"campaign:ip\"             return TOKEN(TCAMPAIGNIP);
\"flight\"                  return TOKEN(TFLIGHT);
\"flight:ip\"               return TOKEN(TFLIGHTIP);
\"product\"                 return TOKEN(TPRODUCT);
\"product:ip\"              return TOKEN(TPRODUCTIP);
\"[a-zA-Z0-9_-]*\"          SAVE_STRING; return TSTRING;
\'[a-zA-Z0-9_-]*\'          SAVE_STRING; return TSTRING;
[a-zA-Z_][a-zA-Z0-9_-]*     SAVE_IDENTIFIER; return TIDENTIFIER;
[0-9]+                      SAVE_INTEGER; return TINTEGER;
[0-9]+\.[0-9]*              SAVE_FLOAT; return TFLOAT;
.                           printf("Unknown token! (%s)\n", yytext); yyterminate();

%%

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
