%{
  #include <iostream>
  #include <cstdlib>
  #include <cctype>
  #include <cstring>
  #include <string>
  #include <vector>

  #include "formula.cpp"

  // Prototypes to keep the compiler happy
  void yyerror (const char *error);
  int  yylex ();
  Formula* f;
%}

%token IDENT

%union {
  Formula* formula;   /* For the expressions. Since it is a pointer, no problem. */
  char* ident;     /* For the lexical analyser. I tokens */
  ListOfFormulas* list_of_formulas;
}

/* Lets inform Bison about the type of each terminal and non-terminal */
%type <formula>  formula
%type <ident>    IDENT
%type <list_of_formulas> list_of_formulas

/* Precedence information to resolve ambiguity */
%left '|'
%left '&'
%left '>'
%left '<'
%left '='
%%

formula : 
    '(' formula ')'         { $$ = $2; }
  | formula '&' formula     { $$ = new AndOperator($1, $3); f = $$;}
  | formula '|' formula     { $$ = new OrOperator($1, $3); f = $$; }
  | formula '>' formula     { $$ = new ImpliesOperator($1, $3); f = $$; }
  | formula '<' formula     { $$ = new ImpliesOperator($3, $1); f = $$; }
  | formula '=' formula     { $$ = new EquivalenceOperator($1, $3); f = $$; }
  | '#' IDENT '(' list_of_formulas ')' { $$ = new AtLeastOperator(atoi($2), $4); f = $$; }
  | IDENT                   { $$ = new Variable($1); f = $$; } 

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

bool isIdentChar(char c) {
  return isalnum(c) || c == '_';
}

int yylex()
{
  do {
    auto ch = std::cin.peek();
    if (ch == ' ' || ch == '\n' || ch == '\t') {
      std::cin.get();
    } else if (!isIdentChar(ch)) {
      std::cin.get();
      return ch;            
    } else {
      std::string s;
      while (isIdentChar(std::cin.peek())) {
        s.push_back(std::cin.get());
      }
      yylval.ident = strdup(s.c_str());
      return IDENT;
    }
  } while (!std::cin.eof());

  std::cout << "end of input" << std::endl;
  return -1;
}
