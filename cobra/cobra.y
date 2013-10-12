%{
  #include <iostream>
  #include <cstdlib>
  #include <cctype>
  #include <cstring>
  #include <string>
  #include <vector>

  #include "formula.cpp"

  #define YYERROR_VERBOSE 
  extern "C" int yylex();
  extern "C" int yyparse();
  void yyerror (const char *error);
  Formula* f;
%}

%token T_IDENT
%token T_INT

/* order of precedence from 
 * http://en.wikipedia.org/wiki/Logical_connective#Order_of_precedence 
 */ 
%left T_EQUIV
%left T_IMPLIES, T_IMPLIED
%left T_OR
%left T_AND
%left '!' 

%token T_ATLEAST
%token T_ATMOST
%token T_EXACTLY

%union {
  Formula* formula;   
  ListOfFormulas* list_of_formulas;
  char*    tstr; 
  int      tint;
}

/* Lets inform Bison about the type of each terminal and non-terminal */
%type <formula>  formula
%type <tstr>     T_IDENT
%type <tint>     T_INT
%type <list_of_formulas> list_of_formulas

%%

formula : 
    '(' formula ')'             
      { $$ = $2; }
  | formula T_AND formula  
      { $$ = new AndOperator($1, $3); f = $$; }
  | formula T_OR formula        
      { $$ = new OrOperator($1, $3); f = $$; }
  | formula T_IMPLIES formula   
      { $$ = new ImpliesOperator($1, $3); f = $$; }
  | formula T_IMPLIED formula   
      { $$ = new ImpliesOperator($3, $1); f = $$; } 
  | formula T_EQUIV formula 
      { $$ = new EquivalenceOperator($1, $3); f = $$; }
  | T_ATLEAST '-' T_INT '(' list_of_formulas ')' 
      { $$ = new AtLeastOperator($3, $5); f = $$; }
  | T_ATMOST '-' T_INT '(' list_of_formulas ')' 
      { $$ = new AtMostOperator($3, $5); f = $$; }
  | T_EXACTLY '-' T_INT '(' list_of_formulas ')' 
      { $$ = new ExactlyOperator($3, $5); f = $$; }
  | '!' formula
      { $$ = new NotOperator($2); f = $$; }
  | T_IDENT       
      { $$ = new Variable($1); f = $$; } 

list_of_formulas :
    formula    { $$ = new ListOfFormulas($1); }
  | list_of_formulas ',' formula { $1->add($3); }

%%


int main()
{
  yyparse();
  f->dump();
  printf(".\n");
  delete f;
  return 0;
}

void yyerror(const char *error)
{
  std::cout << error << std::endl;
}