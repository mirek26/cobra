#include <vector>
#include "include/gtest/gtest.h"
#include "../formula.h"
#include "../solver.h"
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

TEST(FormulaTest, ExpandExactlyNone) {
  auto f1 = Formula::Parse("Exactly-0(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  EXPECT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  EXPECT_STREQ(s.c_str(), "((!a1) & (!a2) & (!a3))");
}

TEST(FormulaTest, ExpandExactlyAll) {
  auto f1 = Formula::Parse("Exactly-3(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  EXPECT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  EXPECT_STREQ(s.c_str(), "((a1) & (a2) & (a3))");
}

TEST(FormulaSubstitude, SimpleSubstitude) {
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

TEST(FormulaSubstitude, FormulaListSubstitude) {
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

TEST(SolverTest, BasicSatisfiability) {
  Solver s;
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
  Solver s;
  s.AddConstraint(Formula::Parse("Exactly-2(x1..x5)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1..x3)"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(s.GetFixedVariables(NULL), 2); // x4 and x5 must be false.
}


