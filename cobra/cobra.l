%{
#include "formula.cpp"
#include "cobra.tab.hpp"

#define YY_DECL extern "C" int yylex()
%}
%%

 /* white space */
[ \t\n]         ;
 /* comments */  
\#.*\n			;  /**/

 /* number constants */
[0-9]+             { yylval.tint = atoi(yytext); return T_INT; }

 /* operators */
-\>				{ return T_IMPLIES; }
\<-				{ return T_IMPLIED; }
\<-\>		    { return T_EQUIV; }
\& 				{ return T_AND; }
"and" 			{ return T_AND; }
\| 				{ return T_OR; }
"or" 			{ return T_OR; }

"AtLeast"		{ return T_ATLEAST; }
"AtMost"		{ return T_ATMOST; }
"Exactly"		{ return T_EXACTLY; }


 /* identifiers */
[a-zA-Z][a-zA-Z0-9_]* { yylval.tstr = strdup(yytext); return T_IDENT; }

 /* default: return char on input */
 .				{ return yytext[0]; }
%%