/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
     T_OUTCOME = 278
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 55 "src/cobra.ypp"

  Formula* formula;
  Variable* variable;
  std::vector<Variable*>* variable_list;
  std::vector<Formula*>* formula_list;
  std::vector<std::string>* string_list;
  std::vector<uint>* int_list;
  char* tstr;
  uint tint;



/* Line 2068 of yacc.c  */
#line 86 "src/cobra.tab.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


