%option yylineno
%option noyywrap
%{
#include "formula.h"
#include "game.h"
#include "cobra.tab.hpp"

extern "C" int yyparse();
#define YY_DECL extern "C" int yylex()

char* str_dup_strip(char* str) {
  char* result = (char*) malloc(strlen(yytext)-1);
  strncpy(result, yytext + 1, strlen(yytext)-1);
  result[strlen(yytext)-2] = '\0';
  return result;
}

%}
%%

 /* white space */
[ \t\n]         ;
 /* comments */
\#.*\n          ;  /**/

 /* number constants */
[0-9]+             { yylval.tint = atoi(yytext); return T_INT; }

 /* operators */
-\>             { return T_IMPLIES; }
→               { return T_IMPLIES; }
⇒               { return T_IMPLIES; }
\<-             { return T_IMPLIED; }
←               { return T_IMPLIED; }
\<-\>           { return T_EQUIV; }
⇔               { return T_EQUIV; }
↔               { return T_EQUIV; }
\&\&            { return T_AND; }
\&              { return T_AND; }
∧               { return T_AND; }
[aA][nN][dD]    { return T_AND; }
\|\|            { return T_OR; }
\|              { return T_OR; }
∨               { return T_OR; }
[oO][rR]        { return T_OR; }
¬               { return T_NOT; }
!               { return T_NOT; }

"AtLeast"       { return T_ATLEAST; }
"AtMost"        { return T_ATMOST; }
"Exactly"       { return T_EXACTLY; }

"VARIABLE"      { return T_VARIABLE; }
"VARIABLES"     { return T_VARIABLES; }
"RESTRICTION"   { return T_RESTRICTION; }
"ALPHABET"      { return T_ALPHABET; }
"MAPPING"       { return T_MAPPING; }
"EXPERIMENT"    { return T_EXPERIMENT; }
"PARAMS_DISTINCT" { return T_PARAMS_DISTINCT; }
"PARAMS_SORTED" { return T_PARAMS_SORTED; }
"OUTCOME"       { return T_OUTCOME; }

 /* identifiers */
[a-zA-Z_][a-zA-Z0-9_]* { yylval.tstr = strdup(yytext); return T_IDENT; }
\"[^"]*\" { yylval.tstr = str_dup_strip(yytext); return T_STRING; }
\'[^']*\' { yylval.tstr = str_dup_strip(yytext); return T_STRING; }

 /* default: return char on input */
 .              { return yytext[0]; }
%%

void parse_string(std::string str) {
  yy_switch_to_buffer(yy_scan_string(str.c_str()));
  yyparse();
}
