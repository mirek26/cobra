/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_IDENT = 258,
     T_INT = 259,
     T_STRING = 260,
     T_EQUIV = 261,
     T_IMPLIES = 262,
     T_IMPLIED = 263,
     T_OR = 264,
     T_AND = 265,
     T_NOT = 266,
     T_ATLEAST = 267,
     T_ATMOST = 268,
     T_EXACTLY = 269,
     T_VARIABLE = 270,
     T_VARIABLES = 271,
     T_RESTRICTION = 272,
     T_ALPHABET = 273,
     T_MAPPING = 274,
     T_EXPERIMENT = 275,
     T_PARAMS_DISTINCT = 276,
     T_PARAMS_SORTED = 277,
     T_OUTCOME = 278,
     T_OUTCOMEX = 279
   };
#endif
/* Tokens.  */
#define T_IDENT 258
#define T_INT 259
#define T_STRING 260
#define T_EQUIV 261
#define T_IMPLIES 262
#define T_IMPLIED 263
#define T_OR 264
#define T_AND 265
#define T_NOT 266
#define T_ATLEAST 267
#define T_ATMOST 268
#define T_EXACTLY 269
#define T_VARIABLE 270
#define T_VARIABLES 271
#define T_RESTRICTION 272
#define T_ALPHABET 273
#define T_MAPPING 274
#define T_EXPERIMENT 275
#define T_PARAMS_DISTINCT 276
#define T_PARAMS_SORTED 277
#define T_OUTCOME 278
#define T_OUTCOMEX 279




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 56 "src/cobra.ypp"
{
  Formula* formula;
  vec<Variable*>* variable_list;
  vec<Formula*>* formula_list;
  vec<string>* string_list;
  vec<uint>* int_list;
  char* tstr;
  uint tint;
}
/* Line 1529 of yacc.c.  */
#line 107 "src/cobra.tab.hpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

