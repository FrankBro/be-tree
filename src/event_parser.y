%{
    #include <stdint.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <string.h>
    #include "ast.h"
    #include "betree.h"
    #include "event_parser.h"
    #include "value.h"
    struct event *root;
    extern int zzlex();
    void zzerror(void *scanner, const char *s) { (void)scanner; printf("ERROR: %s\n", s); }
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
%define api.prefix {zz}

%{
    int event_parse(const char *text, struct event **event);
%}

%union {
    int token;
    char *string;

    bool boolean_value;
    int64_t integer_value;
    double float_value;
    struct string_value string_value;
    struct integer_list_value integer_list_value;
    struct string_list_value string_list_value;
    struct segments_list segments_list_value;
    struct segment segment_value;
    struct frequency_caps_list frequencies_value;
    struct frequency_cap frequency_value;

    struct value value;
    struct pred* pred;

    struct event* event;
}

%token<token> EVENT_LCURLY EVENT_RCURLY
%token<token> EVENT_LSQUARE EVENT_RSQUARE
%token<token> EVENT_COMMA
%token<token> EVENT_COLON
%token<token> EVENT_MINUS
%token<token> EVENT_NULL

%token<boolean_value> EVENT_TRUE EVENT_FALSE
%type<boolean_value> boolean
%token<integer_value> EVENT_INTEGER 
%type<integer_value> integer
%token<float_value> EVENT_FLOAT 
%type<float_value> float
%token<string> EVENT_STRING
%type<string_value> string
%type<integer_list_value> integer_list_value integer_list_loop
%type<string_list_value> string_list_value string_list_loop
%type<segments_list_value> segments_value segments_loop
%type<segment_value> segment_value
%type<frequencies_value> frequencies_value frequencies_loop
%type<frequency_value> frequency_value;

%type<value> value
%type<pred> pred
%type<event> pred_loop

%start program

%printer { fprintf(yyoutput, "%lld", $$); } <integer>
%printer { fprintf(yyoutput, "%lld", $$); } <integer_value>
%printer { fprintf(yyoutput, "%.2f", $$); } <float>
%printer { fprintf(yyoutput, "%.2f", $$); } <float_value>
%printer { fprintf(yyoutput, "%s", $$); } <string>
%printer { fprintf(yyoutput, "%s", $$.string); } <string_value>
%printer { fprintf(yyoutput, "%zu integers", $$.count); } <integer_list_value>
%printer { fprintf(yyoutput, "%zu strings", $$.count); } <string_list_value>
%printer { fprintf(yyoutput, "%zu segments", $$.size); } <segments_list_value>
%printer { fprintf(yyoutput, "%zu caps", $$.size); } <frequencies_value>

%%

program             : EVENT_LCURLY pred_loop EVENT_RCURLY   { root = $2; }

pred_loop           : pred                                  { $$ = make_event(); add_pred($1, $$); }
                    | pred_loop EVENT_COMMA pred            { add_pred($3, $1); $$ = $1; }
;       

pred                : EVENT_STRING EVENT_COLON value        { $$ = make_pred($1, -1, $3); free($1); }
                    | EVENT_STRING EVENT_COLON EVENT_NULL   { $$ = NULL; free($1); }
;

value               : boolean                               { $$.value_type = VALUE_B; $$.bvalue = $1; }
                    | integer                               { $$.value_type = VALUE_I; $$.ivalue = $1; }
                    | float                                 { $$.value_type = VALUE_F; $$.fvalue = $1; }
                    | string                                { $$.value_type = VALUE_S; $$.svalue = $1; }
                    | integer_list_value                    { $$.value_type = VALUE_IL; $$.ilvalue = $1; }
                    | string_list_value                     { $$.value_type = VALUE_SL; $$.slvalue = $1; }
                    | segments_value                        { $$.value_type = VALUE_SEGMENTS; $$.segments_value = $1; }
                    | frequencies_value                     { $$.value_type = VALUE_FREQUENCY; $$.frequency_value = $1; }

boolean             : EVENT_TRUE                            { $$ = true; }
                    | EVENT_FALSE                           { $$ = false; }
;                       

integer             : EVENT_INTEGER                         { $$ = $1; }
                    | EVENT_MINUS EVENT_INTEGER             { $$ = - $2; }
;                       

float               : EVENT_FLOAT                           { $$ = $1; }
                    | EVENT_MINUS EVENT_FLOAT               { $$ = - $2; }
;       

string              : EVENT_STRING                          { $$.string = strdup($1); $$.str = -1; free($1); }

integer_list_value  : EVENT_LSQUARE integer_list_loop EVENT_RSQUARE       
                                                            { $$ = $2; }
                    | EVENT_LSQUARE EVENT_RSQUARE           { $$.count = 0; $$.integers = NULL; }
;

integer_list_loop   : integer                               { $$.count = 0; $$.integers = NULL; add_integer_list_value($1, &$$); }
                    | integer_list_loop EVENT_COMMA integer { add_integer_list_value($3, &$1); $$ = $1; }
;               

string_list_value   : EVENT_LSQUARE string_list_loop EVENT_RSQUARE        
                                                            { $$ = $2; }
                    | EVENT_LSQUARE EVENT_RSQUARE           { $$.count = 0; $$.strings = NULL; }
;

string_list_loop    : string                                { $$.count = 0; $$.strings = NULL; add_string_list_value($1, &$$); }
                    | string_list_loop EVENT_COMMA string   { add_string_list_value($3, &$1); $$ = $1; }
;       

segments_value      : EVENT_LSQUARE segments_loop EVENT_RSQUARE           
                                                            { $$ = $2; }
                    | EVENT_LSQUARE EVENT_RSQUARE           { $$.size = 0; $$.content = NULL; }
;

segments_loop       : segment_value                         { $$.size = 0; $$.content = NULL; add_segment($1, &$$); }
                    | segments_loop EVENT_COMMA segment_value        
                                                            { add_segment($3, &$1); $$ = $1; }
;

segment_value       : EVENT_LSQUARE integer EVENT_COMMA integer EVENT_RSQUARE  
                                                            { $$ = make_segment($2, $4); }

frequencies_value   : EVENT_LSQUARE frequencies_loop EVENT_RSQUARE
                                                            { $$ = $2; }
                    | EVENT_LSQUARE EVENT_RSQUARE           { $$.size = 0; $$.content = NULL; }
;

frequencies_loop    : frequency_value                       { $$.size = 0; $$.content = NULL; add_frequency($1, &$$); }
                    | frequencies_loop EVENT_COMMA frequency_value  
                                                            { add_frequency($3, &$1); $$ = $1; }
;

frequency_value     : EVENT_LSQUARE EVENT_STRING EVENT_COMMA integer EVENT_COMMA string EVENT_COMMA integer EVENT_COMMA integer EVENT_RSQUARE
                                                            { $$ = make_frequency_cap($2, $4, $6, $8, $10); free($2); }

%%

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "event_lexer.h"

int event_parse(const char *text, struct event** event)
{
    // zzdebug = 1;
    
    // Parse using Bison.
    yyscan_t scanner;
    zzlex_init(&scanner);
    YY_BUFFER_STATE buffer = zz_scan_string(text, scanner);
    int rc = zzparse(scanner);
    zz_delete_buffer(buffer, scanner);
    zzlex_destroy(scanner);
    
    if(rc == 0) {
        *event = root;
        return 0;
    }
    else {
        return -1;
    }
}
