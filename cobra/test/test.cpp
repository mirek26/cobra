#include <vector>
#include "include/gtest/gtest.h"
#include "../formula.h"
#include "../cnf-formula.h"
#include "../parser.h"

extern Parser m;

TEST(ParserGetTest, GetVariableWithSameName) {
  std::string s1 = "test";
  std::string s2 = "test";
  auto v1 = m.get<Variable>(s1);
  auto v2 = m.get<Variable>(s2);
  EXPECT_EQ(v1, v2);
}

TEST(ParserGetTest, GetVariableWithSameParams) {
  std::vector<int> params1 = { 42, 0, 18 };
  std::vector<int> params2(params1);
  std::string s = "test";
  auto v1 = m.get<Variable>(s, params1);
  auto v2 = m.get<Variable>(s, params2);
  EXPECT_EQ(v1, v2);
}

TEST(ParserGetTest, GetVariableWithNoParams) {
  std::vector<int> params1;
  std::vector<int> params2;
  std::string s = "test";
  auto v1 = m.get<Variable>(s, params1);
  auto v2 = m.get<Variable>(s, params2);
  EXPECT_EQ(v1, v2);
}

TEST(FormulaTest, ParamEqTest) {
  auto f1 = Formula::Parse("p1==x1 -> (q1<->y1)");
  EXPECT_STREQ("((p1 == x1) -> (q1 <-> y1))", f1->pretty(false).c_str());
}

TEST(FormulaTest, ExpandExactlyNone) {
  auto f1 = Formula::Parse("Exactly-0(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  EXPECT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  EXPECT_STREQ("((!a1) & (!a2) & (!a3))", s.c_str());
}

TEST(FormulaTest, ExpandExactlyAll) {
  auto f1 = Formula::Parse("Exactly-3(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  EXPECT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  EXPECT_STREQ("((a1) & (a2) & (a3))", s.c_str());
}

TEST(FormulaSubstitute, SimpleSubstitute) {
  std::string a = "a", b = "b", c = "c", x = "x", y = "y", z = "z";
  auto f1 = Formula::Parse("a & b | c");
  std::map<Variable*, Variable*> table;
  table[m.get<Variable>(a)] = m.get<Variable>(x);
  table[m.get<Variable>(b)] = m.get<Variable>(y);
  table[m.get<Variable>(c)] = m.get<Variable>(z);
  auto f2 = f1->Substitude(table);
  EXPECT_NE(f1, f2);
  EXPECT_STREQ("((x & y) | z)", f2->pretty(false).c_str());
}

TEST(FormulaSubstitute, FormulaListSubstitute) {
  std::string a = "a", b = "b", c = "c", x = "x", y = "y", z = "z";
  auto f1 = Formula::Parse("Exactly-1(AtLeast-2(a, b, c), AtMost-2(x, y, z))");
  std::map<Variable*, Variable*> table;
  table[m.get<Variable>(a)] = m.get<Variable>(x);
  table[m.get<Variable>(b)] = m.get<Variable>(y);
  table[m.get<Variable>(c)] = m.get<Variable>(z);
  table[m.get<Variable>(x)] = m.get<Variable>(c);
  table[m.get<Variable>(y)] = m.get<Variable>(b);
  table[m.get<Variable>(z)] = m.get<Variable>(a);
  auto f2 = f1->Substitude(table);
  EXPECT_NE(f1, f2);
  EXPECT_STREQ("Exactly-1(AtLeast-2(x, y, z), AtMost-2(c, b, a))",
               f2->pretty(false).c_str());
}

TEST(FormulaList, Range1) {
  auto f = Formula::Parse("Or(x_I / I = 1..4)");
  EXPECT_STREQ("(x1 | x2 | x3 | x4)", f->pretty(false).c_str());
  f = Formula::Parse("Or(x_I / I = 1..3, x4)");
  EXPECT_STREQ("(x1 | x2 | x3 | x4)", f->pretty(false).c_str());
  f = Formula::Parse("Or(x_1, x_I / I = 2..4)");
  EXPECT_STREQ("(x1 | x2 | x3 | x4)", f->pretty(false).c_str());
  f = Formula::Parse("Or(x_I / I = 1..2, x_J / J = 3..4)");
  EXPECT_STREQ("(x1 | x2 | x3 | x4)", f->pretty(false).c_str());
}

TEST(FormulaList, Range2) {
  auto f = Formula::Parse("And(x_I | y_I / I = 1..4)");
  EXPECT_STREQ("((x1 | y1) & (x2 | y2) & (x3 | y3) & (x4 | y4))",
               f->pretty(false).c_str());
  f = Formula::Parse("And(Or(x_I_J / I = 1..2) / J = 1..2)");
  EXPECT_STREQ("((x1_1 | x2_1) & (x1_2 | x2_2))",
               f->pretty(false).c_str());
}

TEST(FormulaList, MultiRange) {
  auto f = Formula::Parse("And(Or(x_I, y_J, z_K) / I = 1..2 / J = 1..2 / K = 1..2)");
  EXPECT_STREQ("((x1 | y1 | z1) & (x1 | y1 | z2) & (x1 | y2 | z1) &"
    " (x1 | y2 | z2) & (x2 | y1 | z1) & (x2 | y1 | z2) & (x2 | y2 | z1)"
    " & (x2 | y2 | z2))", f->pretty(false).c_str());
}

TEST(CnfSubstitude, ParamEq) {
  auto f1 = Formula::Parse("(p1==a -> p1 | x) && (p1==b -> p1 & x)");
  auto cnf = f1->ToCnf();
  std::vector<Variable*> v1, v2;
  std::string a = "a", b = "b";
  v1.push_back(m.get<Variable>(a));
  v2.push_back(m.get<Variable>(b));
  auto p1 = cnf->SubstituteParams(v1);
  auto p2 = cnf->SubstituteParams(v2);
  //EXPECT_STREQ("a | x", p1.pretty(false).c_str());
  //EXPECT_STREQ("b & x", p2.pretty(false).c_str());
}

TEST(SolverTest, BasicSatisfiability) {
  CnfFormula s;
  s.AddConstraint(Formula::Parse("a -> b"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("c -> d"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("!b & !d"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("a & c"));
  EXPECT_FALSE(s.Satisfiable());
}

TEST(SolverTest, BasicFixed) {
  CnfFormula s;
  s.AddConstraint(Formula::Parse("Exactly-2(x1..x5)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1..x3)"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(s.GetFixedVariables(), 2); // x4 and x5 must be false.
}



