/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         XXSTYPE
/* Substitute the variable and function names.  */
#define yyparse         xxparse
#define yylex           xxlex
#define yyerror         xxerror
#define yydebug         xxdebug
#define yynerrs         xxnerrs


/* Copy the first part of user declarations.  */
#line 1 "src/parser.y" /* yacc.c:339  */

    #include <stdint.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <string.h>
    #include "alloc.h"
    #include "ast.h"
    #include "betree.h"
    #include "parser.h"
    #include "value.h"
    extern int xxlex();
    void xxerror(void *scanner, struct ast_node** node, const char *s) { (void)node; (void)scanner; printf("ERROR: %s\n", s); }
#ifdef NIF
    #include "erl_nif.h"
    #define YYMALLOC enif_alloc
    #define YYFREE enif_free
#endif
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-default"
    #pragma GCC diagnostic ignored "-Wshadow"
#endif
#line 31 "src/parser.y" /* yacc.c:339  */

    int parse(const char *text, struct ast_node **node);

#line 100 "src/parser.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parser.h".  */
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
#line 35 "src/parser.y" /* yacc.c:355  */

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

#line 238 "src/parser.c" /* yacc.c:355  */
};

typedef union XXSTYPE XXSTYPE;
# define XXSTYPE_IS_TRIVIAL 1
# define XXSTYPE_IS_DECLARED 1
#endif



int xxparse (void *scanner, struct ast_node** root);

#endif /* !YY_XX_SRC_PARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 254 "src/parser.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined XXSTYPE_IS_TRIVIAL && XXSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  49
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   187

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  37
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  76
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  168

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   291

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36
};

#if XXDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    96,    96,    98,   100,   101,   104,   105,   108,   110,
     112,   113,   116,   118,   119,   122,   123,   124,   125,   126,
     127,   128,   129,   132,   133,   134,   137,   138,   141,   142,
     143,   144,   145,   146,   147,   148,   151,   152,   153,   156,
     157,   158,   159,   162,   164,   165,   166,   169,   170,   171,
     174,   175,   178,   179,   182,   183,   184,   187,   188,   189,
     190,   191,   192,   195,   196,   197,   198,   201,   205,   207,
     209,   211,   215,   217,   221,   223,   225
};
#endif

#if XXDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TMINUS", "TCEQ", "TCNE", "TCGT", "TCGE",
  "TCLT", "TCLE", "TLPAREN", "TRPAREN", "TCOMMA", "TNOTIN", "TIN",
  "TONEOF", "TNONEOF", "TALLOF", "TAND", "TOR", "TNOT",
  "TWITHINFREQUENCYCAP", "TSEGMENTWITHIN", "TSEGMENTBEFORE",
  "TGEOWITHINRADIUS", "TCONTAINS", "TSTARTSWITH", "TENDSWITH",
  "TISNOTNULL", "TISNULL", "TISEMPTY", "TTRUE", "TFALSE", "TSTRING",
  "TIDENTIFIER", "TINTEGER", "TFLOAT", "$accept", "program", "ident",
  "integer", "float", "string", "integer_list_value", "integer_list_loop",
  "string_list_value", "string_list_loop", "expr", "is_null_expr",
  "num_comp_value", "num_comp_expr", "eq_value", "eq_expr",
  "variable_value", "set_left_value", "set_right_value", "set_expr",
  "list_value", "list_expr", "bool_expr", "special_expr",
  "s_frequency_expr", "s_segment_expr", "s_geo_expr", "s_string_expr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291
};
# endif

#define YYPACT_NINF -44

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-44)))

#define YYTABLE_NINF -46

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      15,    -8,    15,    15,    -5,     7,    21,    33,    47,    76,
      93,   -44,   -44,   -44,   -44,   -44,   -44,   105,   134,    65,
      63,    41,    53,   -44,    87,   -44,    77,   -44,   -44,    86,
     -44,   -44,   -44,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
      73,   -44,     0,    10,    10,    26,    83,    83,    83,   -44,
      23,    23,    26,    26,    26,    26,   101,   101,   101,   -44,
     -44,   -44,    15,    15,    83,    83,    83,    83,    83,    83,
      -4,    -4,   -44,    85,    72,   144,   145,   146,   147,   148,
     149,   153,   154,   155,   -44,   -44,   -44,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,   -44,    50,   -44,   -44,   -44,   -44,
     -44,   -44,   128,   -44,   -44,   -44,   -44,   -44,   -44,   -44,
     -44,   -44,   -44,   -44,   -44,   135,    31,    31,    31,    31,
      31,    29,   135,   135,   135,   -44,   -44,   104,   113,   157,
     158,   160,   161,   163,   164,   136,   165,   167,   168,   169,
     -44,    31,   -44,   135,    31,    31,   -44,    31,   -44,    31,
      29,   -44,   -44,   -44,   -44,   -44,   170,   172,   173,   174,
     175,    31,   -44,   -44,   -44,   -44,   176,   -44
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    61,    62,     8,     3,     4,     6,     0,    60,    26,
      27,    38,     2,    22,     0,    16,     0,    17,    46,     0,
      18,    19,    20,    21,    63,    64,    65,    66,     5,     7,
       0,    59,     0,     0,     0,     0,     0,     0,     0,     1,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    24,
      23,    25,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    15,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    36,    37,    38,    39,    40,    26,
      27,    28,    29,    30,    31,     0,    52,    53,    54,    55,
      56,    57,    58,    34,    35,    32,    33,    41,    42,    43,
      47,    48,    49,    50,    51,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    10,    13,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       9,     0,    12,     0,     0,     0,    68,     0,    70,     0,
       0,    74,    75,    76,    11,    14,     0,     0,     0,     0,
       0,     0,    69,    71,    72,    73,     0,    67
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -44,   -44,    66,   -43,   -31,   -35,    56,   -44,    58,   -44,
       1,   -44,    68,   -44,    94,   -44,    82,   -44,   110,   -44,
      97,   -44,   -44,   -44,   -44,   -44,   -44,   -44
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    17,    18,    19,    20,    21,    96,   127,    97,   128,
      22,    23,    24,    25,    26,    27,    28,    29,   113,    30,
      98,    31,    32,    33,    34,    35,    36,    37
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      76,    78,    79,    40,    41,    42,    95,    84,    84,    89,
      89,    89,    89,    74,    80,    86,    86,    43,     1,    85,
      85,    90,    90,    90,    90,     2,     1,    38,    39,     1,
      14,    44,   135,    73,    74,     3,     4,     5,     6,     7,
       8,     9,    10,    45,    14,    15,    11,    12,    13,    14,
      15,    16,   125,    74,   -45,   -45,    13,    46,    15,    16,
     126,    15,    16,   101,   102,    16,    15,   -37,   -37,   -36,
     -36,    62,    63,   130,   131,   132,   133,   134,   -44,   -44,
     129,    68,    69,    13,    72,    15,    47,   137,   138,   139,
     136,    62,    63,    64,    65,    66,    67,   115,   154,    70,
      71,   156,   157,    48,   158,    49,   159,    38,   155,    75,
      77,    95,    81,    82,    83,   140,   141,    14,   166,   160,
      91,    92,    93,    94,   142,   143,   110,   110,   111,   111,
     103,   104,   105,   106,   107,   108,   109,   109,    50,    51,
      52,    53,    54,    55,    87,    88,    62,   -43,   -43,    56,
      57,    58,   112,   112,    99,   100,   116,   117,   118,   119,
     120,   121,    59,    60,    61,   122,   123,   124,    13,   144,
     145,   146,    39,   147,   148,     0,   149,   150,   151,   152,
     153,   114,   161,   162,   163,   164,   165,   167
};

static const yytype_int16 yycheck[] =
{
      43,    44,    45,     2,     3,    10,    10,    50,    51,    52,
      53,    54,    55,     3,    45,    50,    51,    10,     3,    50,
      51,    52,    53,    54,    55,    10,     3,    35,    36,     3,
      34,    10,     3,    33,     3,    20,    21,    22,    23,    24,
      25,    26,    27,    10,    34,    35,    31,    32,    33,    34,
      35,    36,    95,     3,    13,    14,    33,    10,    35,    36,
      95,    35,    36,    62,    63,    36,    35,     4,     5,     4,
       5,    18,    19,   116,   117,   118,   119,   120,    13,    14,
     115,     4,     5,    33,    11,    35,    10,   122,   123,   124,
     121,    18,    19,     6,     7,     8,     9,    12,   141,    13,
      14,   144,   145,    10,   147,     0,   149,    35,   143,    43,
      44,    10,    46,    47,    48,    11,    12,    34,   161,   150,
      52,    53,    54,    55,    11,    12,    70,    71,    70,    71,
      64,    65,    66,    67,    68,    69,    70,    71,     4,     5,
       6,     7,     8,     9,    50,    51,    18,    13,    14,    15,
      16,    17,    70,    71,    57,    58,    12,    12,    12,    12,
      12,    12,    28,    29,    30,    12,    12,    12,    33,    12,
      12,    11,    36,    12,    11,    -1,    12,    12,    11,    11,
      11,    71,    12,    11,    11,    11,    11,    11
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,    10,    20,    21,    22,    23,    24,    25,    26,
      27,    31,    32,    33,    34,    35,    36,    38,    39,    40,
      41,    42,    47,    48,    49,    50,    51,    52,    53,    54,
      56,    58,    59,    60,    61,    62,    63,    64,    35,    36,
      47,    47,    10,    10,    10,    10,    10,    10,    10,     0,
       4,     5,     6,     7,     8,     9,    15,    16,    17,    28,
      29,    30,    18,    19,     6,     7,     8,     9,     4,     5,
      13,    14,    11,    33,     3,    39,    40,    39,    40,    40,
      41,    39,    39,    39,    40,    41,    42,    51,    51,    40,
      41,    49,    49,    49,    49,    10,    43,    45,    57,    57,
      57,    47,    47,    39,    39,    39,    39,    39,    39,    39,
      43,    45,    53,    55,    55,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    40,    42,    44,    46,    42,
      40,    40,    40,    40,    40,     3,    41,    42,    42,    42,
      11,    12,    11,    12,    12,    12,    11,    12,    11,    12,
      12,    11,    11,    11,    40,    42,    40,    40,    40,    40,
      41,    12,    11,    11,    11,    11,    40,    11
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    37,    38,    39,    40,    40,    41,    41,    42,    43,
      44,    44,    45,    46,    46,    47,    47,    47,    47,    47,
      47,    47,    47,    48,    48,    48,    49,    49,    50,    50,
      50,    50,    50,    50,    50,    50,    51,    51,    51,    52,
      52,    52,    52,    53,    54,    54,    54,    55,    55,    55,
      56,    56,    57,    57,    58,    58,    58,    59,    59,    59,
      59,    59,    59,    60,    60,    60,    60,    61,    62,    62,
      62,    62,    63,    63,    64,    64,    64
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     1,     2,     1,     3,
       1,     3,     3,     1,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     1,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     1,     1,     3,
       3,     3,     3,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     1,     1,     3,     3,     3,     3,     3,     2,
       1,     1,     1,     1,     1,     1,     1,    10,     6,     8,
       6,     8,     8,     8,     6,     6,     6
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (scanner, root, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if XXDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, scanner, root); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner, struct ast_node** root)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (scanner);
  YYUSE (root);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner, struct ast_node** root)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner, root);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, void *scanner, struct ast_node** root)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , scanner, root);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner, root); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !XXDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !XXDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *scanner, struct ast_node** root)
{
  YYUSE (yyvaluep);
  YYUSE (scanner);
  YYUSE (root);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void *scanner, struct ast_node** root)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, scanner, root);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 96 "src/parser.y" /* yacc.c:1646  */
    { *root = (yyvsp[0].node); }
#line 1454 "src/parser.c" /* yacc.c:1646  */
    break;

  case 3:
#line 98 "src/parser.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 1460 "src/parser.c" /* yacc.c:1646  */
    break;

  case 4:
#line 100 "src/parser.y" /* yacc.c:1646  */
    { (yyval.integer_value) = (yyvsp[0].integer_value); }
#line 1466 "src/parser.c" /* yacc.c:1646  */
    break;

  case 5:
#line 101 "src/parser.y" /* yacc.c:1646  */
    { (yyval.integer_value) = - (yyvsp[0].integer_value); }
#line 1472 "src/parser.c" /* yacc.c:1646  */
    break;

  case 6:
#line 104 "src/parser.y" /* yacc.c:1646  */
    { (yyval.float_value) = (yyvsp[0].float_value); }
#line 1478 "src/parser.c" /* yacc.c:1646  */
    break;

  case 7:
#line 105 "src/parser.y" /* yacc.c:1646  */
    { (yyval.float_value) = - (yyvsp[0].float_value); }
#line 1484 "src/parser.c" /* yacc.c:1646  */
    break;

  case 8:
#line 108 "src/parser.y" /* yacc.c:1646  */
    { (yyval.string_value).string = bstrdup((yyvsp[0].string)); (yyval.string_value).str = INVALID_STR; bfree((yyvsp[0].string)); }
#line 1490 "src/parser.c" /* yacc.c:1646  */
    break;

  case 9:
#line 110 "src/parser.y" /* yacc.c:1646  */
    { (yyval.integer_list_value) = (yyvsp[-1].integer_list_value); }
#line 1496 "src/parser.c" /* yacc.c:1646  */
    break;

  case 10:
#line 112 "src/parser.y" /* yacc.c:1646  */
    { (yyval.integer_list_value) = make_integer_list(); add_integer_list_value((yyvsp[0].integer_value), (yyval.integer_list_value)); }
#line 1502 "src/parser.c" /* yacc.c:1646  */
    break;

  case 11:
#line 113 "src/parser.y" /* yacc.c:1646  */
    { add_integer_list_value((yyvsp[0].integer_value), (yyvsp[-2].integer_list_value)); (yyval.integer_list_value) = (yyvsp[-2].integer_list_value); }
#line 1508 "src/parser.c" /* yacc.c:1646  */
    break;

  case 12:
#line 116 "src/parser.y" /* yacc.c:1646  */
    { (yyval.string_list_value) = (yyvsp[-1].string_list_value); }
#line 1514 "src/parser.c" /* yacc.c:1646  */
    break;

  case 13:
#line 118 "src/parser.y" /* yacc.c:1646  */
    { (yyval.string_list_value) = make_string_list(); add_string_list_value((yyvsp[0].string_value), (yyval.string_list_value)); }
#line 1520 "src/parser.c" /* yacc.c:1646  */
    break;

  case 14:
#line 119 "src/parser.y" /* yacc.c:1646  */
    { add_string_list_value((yyvsp[0].string_value), (yyvsp[-2].string_list_value)); (yyval.string_list_value) = (yyvsp[-2].string_list_value); }
#line 1526 "src/parser.c" /* yacc.c:1646  */
    break;

  case 15:
#line 122 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[-1].node); }
#line 1532 "src/parser.c" /* yacc.c:1646  */
    break;

  case 16:
#line 123 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1538 "src/parser.c" /* yacc.c:1646  */
    break;

  case 17:
#line 124 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1544 "src/parser.c" /* yacc.c:1646  */
    break;

  case 18:
#line 125 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1550 "src/parser.c" /* yacc.c:1646  */
    break;

  case 19:
#line 126 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1556 "src/parser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 127 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1562 "src/parser.c" /* yacc.c:1646  */
    break;

  case 21:
#line 128 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1568 "src/parser.c" /* yacc.c:1646  */
    break;

  case 22:
#line 129 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1574 "src/parser.c" /* yacc.c:1646  */
    break;

  case 23:
#line 132 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_is_null_expr_create(AST_IS_NULL, (yyvsp[-1].string)); bfree((yyvsp[-1].string)); }
#line 1580 "src/parser.c" /* yacc.c:1646  */
    break;

  case 24:
#line 133 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_is_null_expr_create(AST_IS_NOT_NULL, (yyvsp[-1].string)); bfree((yyvsp[-1].string)); }
#line 1586 "src/parser.c" /* yacc.c:1646  */
    break;

  case 25:
#line 134 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_is_null_expr_create(AST_IS_EMPTY, (yyvsp[-1].string)); bfree((yyvsp[-1].string)); }
#line 1592 "src/parser.c" /* yacc.c:1646  */
    break;

  case 26:
#line 137 "src/parser.y" /* yacc.c:1646  */
    { (yyval.compare_value).value_type = AST_COMPARE_VALUE_INTEGER; (yyval.compare_value).integer_value = (yyvsp[0].integer_value); }
#line 1598 "src/parser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 138 "src/parser.y" /* yacc.c:1646  */
    { (yyval.compare_value).value_type = AST_COMPARE_VALUE_FLOAT; (yyval.compare_value).float_value = (yyvsp[0].float_value); }
#line 1604 "src/parser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 141 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_GT, (yyvsp[-2].string), (yyvsp[0].compare_value)); bfree((yyvsp[-2].string)); }
#line 1610 "src/parser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 142 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_GE, (yyvsp[-2].string), (yyvsp[0].compare_value)); bfree((yyvsp[-2].string)); }
#line 1616 "src/parser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 143 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_LT, (yyvsp[-2].string), (yyvsp[0].compare_value)); bfree((yyvsp[-2].string)); }
#line 1622 "src/parser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 144 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_LE, (yyvsp[-2].string), (yyvsp[0].compare_value)); bfree((yyvsp[-2].string)); }
#line 1628 "src/parser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 145 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_GT, (yyvsp[0].string), (yyvsp[-2].compare_value)); bfree((yyvsp[0].string)); }
#line 1634 "src/parser.c" /* yacc.c:1646  */
    break;

  case 33:
#line 146 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_GE, (yyvsp[0].string), (yyvsp[-2].compare_value)); bfree((yyvsp[0].string)); }
#line 1640 "src/parser.c" /* yacc.c:1646  */
    break;

  case 34:
#line 147 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_LT, (yyvsp[0].string), (yyvsp[-2].compare_value)); bfree((yyvsp[0].string)); }
#line 1646 "src/parser.c" /* yacc.c:1646  */
    break;

  case 35:
#line 148 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_compare_expr_create(AST_COMPARE_LE, (yyvsp[0].string), (yyvsp[-2].compare_value)); bfree((yyvsp[0].string)); }
#line 1652 "src/parser.c" /* yacc.c:1646  */
    break;

  case 36:
#line 151 "src/parser.y" /* yacc.c:1646  */
    { (yyval.equality_value).value_type = AST_EQUALITY_VALUE_INTEGER; (yyval.equality_value).integer_value = (yyvsp[0].integer_value); }
#line 1658 "src/parser.c" /* yacc.c:1646  */
    break;

  case 37:
#line 152 "src/parser.y" /* yacc.c:1646  */
    { (yyval.equality_value).value_type = AST_EQUALITY_VALUE_FLOAT; (yyval.equality_value).float_value = (yyvsp[0].float_value); }
#line 1664 "src/parser.c" /* yacc.c:1646  */
    break;

  case 38:
#line 153 "src/parser.y" /* yacc.c:1646  */
    { (yyval.equality_value).value_type = AST_EQUALITY_VALUE_STRING; (yyval.equality_value).string_value = (yyvsp[0].string_value); }
#line 1670 "src/parser.c" /* yacc.c:1646  */
    break;

  case 39:
#line 156 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_equality_expr_create(AST_EQUALITY_EQ, (yyvsp[-2].string), (yyvsp[0].equality_value)); bfree((yyvsp[-2].string)); }
#line 1676 "src/parser.c" /* yacc.c:1646  */
    break;

  case 40:
#line 157 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_equality_expr_create(AST_EQUALITY_NE, (yyvsp[-2].string), (yyvsp[0].equality_value)); bfree((yyvsp[-2].string)); }
#line 1682 "src/parser.c" /* yacc.c:1646  */
    break;

  case 41:
#line 158 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_equality_expr_create(AST_EQUALITY_EQ, (yyvsp[0].string), (yyvsp[-2].equality_value)); bfree((yyvsp[0].string)); }
#line 1688 "src/parser.c" /* yacc.c:1646  */
    break;

  case 42:
#line 159 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_equality_expr_create(AST_EQUALITY_NE, (yyvsp[0].string), (yyvsp[-2].equality_value)); bfree((yyvsp[0].string)); }
#line 1694 "src/parser.c" /* yacc.c:1646  */
    break;

  case 43:
#line 162 "src/parser.y" /* yacc.c:1646  */
    { (yyval.variable_value) = make_attr_var((yyvsp[0].string), NULL); bfree((yyvsp[0].string)); }
#line 1700 "src/parser.c" /* yacc.c:1646  */
    break;

  case 44:
#line 164 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_left_value).value_type = AST_SET_LEFT_VALUE_INTEGER; (yyval.set_left_value).integer_value = (yyvsp[0].integer_value); }
#line 1706 "src/parser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 165 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_left_value).value_type = AST_SET_LEFT_VALUE_STRING; (yyval.set_left_value).string_value = (yyvsp[0].string_value); }
#line 1712 "src/parser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 166 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_left_value).value_type = AST_SET_LEFT_VALUE_VARIABLE; (yyval.set_left_value).variable_value = (yyvsp[0].variable_value); }
#line 1718 "src/parser.c" /* yacc.c:1646  */
    break;

  case 47:
#line 169 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_right_value).value_type = AST_SET_RIGHT_VALUE_INTEGER_LIST; (yyval.set_right_value).integer_list_value = (yyvsp[0].integer_list_value); }
#line 1724 "src/parser.c" /* yacc.c:1646  */
    break;

  case 48:
#line 170 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_right_value).value_type = AST_SET_RIGHT_VALUE_STRING_LIST; (yyval.set_right_value).string_list_value = (yyvsp[0].string_list_value); }
#line 1730 "src/parser.c" /* yacc.c:1646  */
    break;

  case 49:
#line 171 "src/parser.y" /* yacc.c:1646  */
    { (yyval.set_right_value).value_type = AST_SET_RIGHT_VALUE_VARIABLE; (yyval.set_right_value).variable_value = (yyvsp[0].variable_value); }
#line 1736 "src/parser.c" /* yacc.c:1646  */
    break;

  case 50:
#line 174 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_set_expr_create(AST_SET_NOT_IN, (yyvsp[-2].set_left_value), (yyvsp[0].set_right_value)); }
#line 1742 "src/parser.c" /* yacc.c:1646  */
    break;

  case 51:
#line 175 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_set_expr_create(AST_SET_IN, (yyvsp[-2].set_left_value), (yyvsp[0].set_right_value)); }
#line 1748 "src/parser.c" /* yacc.c:1646  */
    break;

  case 52:
#line 178 "src/parser.y" /* yacc.c:1646  */
    { (yyval.list_value).value_type = AST_LIST_VALUE_INTEGER_LIST; (yyval.list_value).integer_list_value = (yyvsp[0].integer_list_value); }
#line 1754 "src/parser.c" /* yacc.c:1646  */
    break;

  case 53:
#line 179 "src/parser.y" /* yacc.c:1646  */
    { (yyval.list_value).value_type = AST_LIST_VALUE_STRING_LIST; (yyval.list_value).string_list_value = (yyvsp[0].string_list_value); }
#line 1760 "src/parser.c" /* yacc.c:1646  */
    break;

  case 54:
#line 182 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_list_expr_create(AST_LIST_ONE_OF, (yyvsp[-2].string), (yyvsp[0].list_value)); bfree((yyvsp[-2].string));}
#line 1766 "src/parser.c" /* yacc.c:1646  */
    break;

  case 55:
#line 183 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_list_expr_create(AST_LIST_NONE_OF, (yyvsp[-2].string), (yyvsp[0].list_value)); bfree((yyvsp[-2].string));}
#line 1772 "src/parser.c" /* yacc.c:1646  */
    break;

  case 56:
#line 184 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_list_expr_create(AST_LIST_ALL_OF, (yyvsp[-2].string), (yyvsp[0].list_value)); bfree((yyvsp[-2].string));}
#line 1778 "src/parser.c" /* yacc.c:1646  */
    break;

  case 57:
#line 187 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_binary_create(AST_BOOL_AND, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1784 "src/parser.c" /* yacc.c:1646  */
    break;

  case 58:
#line 188 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_binary_create(AST_BOOL_OR, (yyvsp[-2].node), (yyvsp[0].node)); }
#line 1790 "src/parser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 189 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_unary_create((yyvsp[0].node)); }
#line 1796 "src/parser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 190 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_variable_create((yyvsp[0].string)); bfree((yyvsp[0].string)); }
#line 1802 "src/parser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 191 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_literal_create(true); }
#line 1808 "src/parser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 192 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_bool_expr_literal_create(false); }
#line 1814 "src/parser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 195 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1820 "src/parser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 196 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1826 "src/parser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 197 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1832 "src/parser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 198 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = (yyvsp[0].node); }
#line 1838 "src/parser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 202 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_frequency_create(AST_SPECIAL_WITHINFREQUENCYCAP, (yyvsp[-7].string), (yyvsp[-5].string_value), (yyvsp[-3].integer_value), (yyvsp[-1].integer_value)); bfree((yyvsp[-7].string)); }
#line 1844 "src/parser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 206 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_segment_create(AST_SPECIAL_SEGMENTWITHIN, NULL, (yyvsp[-3].integer_value), (yyvsp[-1].integer_value)); }
#line 1850 "src/parser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 208 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_segment_create(AST_SPECIAL_SEGMENTWITHIN, (yyvsp[-5].string), (yyvsp[-3].integer_value), (yyvsp[-1].integer_value)); bfree((yyvsp[-5].string)); }
#line 1856 "src/parser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 210 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_segment_create(AST_SPECIAL_SEGMENTBEFORE, NULL, (yyvsp[-3].integer_value), (yyvsp[-1].integer_value)); }
#line 1862 "src/parser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 212 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_segment_create(AST_SPECIAL_SEGMENTBEFORE, (yyvsp[-5].string), (yyvsp[-3].integer_value), (yyvsp[-1].integer_value)); bfree((yyvsp[-5].string)); }
#line 1868 "src/parser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 216 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_geo_create(AST_SPECIAL_GEOWITHINRADIUS, (double)(yyvsp[-5].integer_value), (double)(yyvsp[-3].integer_value), true, (double)(yyvsp[-1].integer_value)); }
#line 1874 "src/parser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 218 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_geo_create(AST_SPECIAL_GEOWITHINRADIUS, (yyvsp[-5].float_value), (yyvsp[-3].float_value), true, (yyvsp[-1].float_value)); }
#line 1880 "src/parser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 222 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_string_create(AST_SPECIAL_CONTAINS, (yyvsp[-3].string), (yyvsp[-1].string_value).string); bfree((yyvsp[-3].string)); bfree((char*)(yyvsp[-1].string_value).string); }
#line 1886 "src/parser.c" /* yacc.c:1646  */
    break;

  case 75:
#line 224 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_string_create(AST_SPECIAL_STARTSWITH, (yyvsp[-3].string), (yyvsp[-1].string_value).string); bfree((yyvsp[-3].string)); bfree((char*)(yyvsp[-1].string_value).string); }
#line 1892 "src/parser.c" /* yacc.c:1646  */
    break;

  case 76:
#line 226 "src/parser.y" /* yacc.c:1646  */
    { (yyval.node) = ast_special_string_create(AST_SPECIAL_ENDSWITH, (yyvsp[-3].string), (yyvsp[-1].string_value).string); bfree((yyvsp[-3].string)); bfree((char*)(yyvsp[-1].string_value).string); }
#line 1898 "src/parser.c" /* yacc.c:1646  */
    break;


#line 1902 "src/parser.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (scanner, root, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (scanner, root, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, scanner, root);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, scanner, root);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, root, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner, root);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, scanner, root);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 229 "src/parser.y" /* yacc.c:1906  */


#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "lexer.h"

int parse(const char *text, struct ast_node **node)
{
    // xxdebug = 1;
    
    // Parse using Bison.
    yyscan_t scanner;
    xxlex_init(&scanner);
    YY_BUFFER_STATE buffer = xx_scan_string(text, scanner);
    int rc = xxparse(scanner, node);
    xx_delete_buffer(buffer, scanner);
    xxlex_destroy(scanner);
    
    if(rc == 0) {
        return 0;
    }
    else {
        return -1;
    }
}
