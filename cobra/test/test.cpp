#include <vector>
#include "include/gtest/gtest.h"
#include "../src/formula.h"
#include "../src/cnf-formula.h"
#include "../src/parser.h"

extern Parser m;

TEST(ParserGetTest, GetVariableWithSameName) {
  std::string s1 = "test";
  std::string s2 = "test";
  auto v1 = m.get<Variable>(s1);
  auto v2 = m.get<Variable>(s2);
  EXPECT_EQ(v1, v2);
}

TEST(FormulaTest, BasicParse) {
  auto f1 = Formula::Parse("p1 & p2 -> (a <-> b)");
  EXPECT_STREQ("((p1 & p2) -> (a <-> b))", f1->pretty(false).c_str());
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
  s.AddConstraint(Formula::Parse("Exactly-2(x1, x2, x3, x4, x5)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1, x2, x3)"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(s.GetFixedVariables(), 2); // x4 and x5 must be false.
}



