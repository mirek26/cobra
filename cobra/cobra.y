%{
  #include <iostream>
  #include <cctype>
  #include <cstring>
  #include <vector>

  #include "formula.cpp"

  // Prototypes to keep the compiler happy
  void yyerror (const char *error);
  int  yylex ();
  Formula* f;
%}

%token IDENT

%union {
  Formula*  formula;   /* For the expressions. Since it is a pointer, no problem. */
  char      ident;  /* For the lexical analyser. I tokens */
}

/* Lets inform Bison about the type of each terminal and non-terminal */
%type <formula>  formula
%type <ident> IDENT

/* Precedence information to resolve ambiguity */
%left '|'
%left '&'
%%

formula : 
    '(' formula ')'      { $$ = $2; }
  | formula '&' formula  { $$ = new AndOperator($1, $3); f = $$;}
  | formula '|' formula  { $$ = new OrOperator($1, $3); f = $$; }
  | IDENT                { $$ = new Variable($1); f = $$; } 
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

int yylex()
{
  do {
    auto ch = std::cin.peek();
    if (isalpha(ch)) {
      std::cin.get();
      yylval.ident = ch;
      return IDENT;
    } else if (ch == '|' || ch == '&' || ch == '(' || ch == ')') {
       std::cin.get();
       return ch;
    } else {
      std::cin.get();
    }
  } while (!std::cin.eof());

  return -1;
}
