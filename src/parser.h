/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_XX_SRC_PARSER_H_INCLUDED
# define YY_XX_SRC_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef XXDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define XXDEBUG 1
#  else
#   define XXDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define XXDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined XXDEBUG */
#if XXDEBUG
extern int xxdebug;
#endif

/* Token type.  */
#ifndef XXTOKENTYPE
# define XXTOKENTYPE
  enum xxtokentype
  {
    TMINUS = 258,
    TCEQ = 259,
    TCNE = 260,
    TCGT = 261,
    TCGE = 262,
    TCLT = 263,
    TCLE = 264,
    TLPAREN = 265,
    TRPAREN = 266,
    TCOMMA = 267,
    TNOTIN = 268,
    TIN = 269,
    TONEOF = 270,
    TNONEOF = 271,
    TALLOF = 272,
    TAND = 273,
    TOR = 274,
    TNOT = 275,
    TWITHINFREQUENCYCAP = 276,
    TSEGMENTWITHIN = 277,
    TSEGMENTBEFORE = 278,
    TGEOWITHINRADIUS = 279,
    TCONTAINS = 280,
    TSTARTSWITH = 281,
    TENDSWITH = 282,
    TISNOTNULL = 283,
    TISNULL = 284,
    TISEMPTY = 285,
    TTRUE = 286,
    TFALSE = 287,
    TSTRING = 288,
    TIDENTIFIER = 289,
    TINTEGER = 290,
    TFLOAT = 291
  };
#endif
/* Tokens.  */
#define TMINUS 258
#define TCEQ 259
#define TCNE 260
#define TCGT 261
#define TCGE 262
#define TCLT 263
#define TCLE 264
#define TLPAREN 265
#define TRPAREN 266
#define TCOMMA 267
#define TNOTIN 268
#define TIN 269
#define TONEOF 270
#define TNONEOF 271
#define TALLOF 272
#define TAND 273
#define TOR 274
#define TNOT 275
#define TWITHINFREQUENCYCAP 276
#define TSEGMENTWITHIN 277
#define TSEGMENTBEFORE 278
#define TGEOWITHINRADIUS 279
#define TCONTAINS 280
#define TSTARTSWITH 281
#define TENDSWITH 282
#define TISNOTNULL 283
#define TISNULL 284
#define TISEMPTY 285
#define TTRUE 286
#define TFALSE 287
#define TSTRING 288
#define TIDENTIFIER 289
#define TINTEGER 290
#define TFLOAT 291

/* Value type.  */
#if ! defined XXSTYPE && ! defined XXSTYPE_IS_DECLARED

union XXSTYPE
{
#line 35 "src/parser.y" /* yacc.c:1909  */

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

#line 152 "src/parser.h" /* yacc.c:1909  */
};

typedef union XXSTYPE XXSTYPE;
# define XXSTYPE_IS_TRIVIAL 1
# define XXSTYPE_IS_DECLARED 1
#endif



int xxparse (void *scanner, struct ast_node** root);

#endif /* !YY_XX_SRC_PARSER_H_INCLUDED  */
