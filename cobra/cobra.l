%option yylineno
%{
#include "formula.h"
#include "game.h"
#include "cobra.tab.hpp"

#define YY_DECL extern "C" int yylex()
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

"Vars"          { return T_VARS; }
"Init"          { return T_INIT; }
"Exp"           { return T_EXP; }

 /* identifiers */
[a-zA-Z][a-zA-Z_]* { yylval.tstr = strdup(yytext); return T_IDENT; }
\".*\" { yylval.tstr = strdup(yytext); return T_STRING; }

 /* default: return char on input */
 .              { return yytext[0]; }
%%