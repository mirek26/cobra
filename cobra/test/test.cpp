#include <vector>
#include <initializer_list>
#include "include/gtest/gtest.h"
#include "../src/formula.h"
#include "../src/cnf-formula.h"
#include "../src/parser.h"

extern Parser m;

TEST(FormulaTest, BasicParse) {
  m.game().declareVariables({"p1", "p2", "a", "b"});
  auto f1 = Formula::Parse("p1 & p2 -> (a <-> b)");
  EXPECT_STREQ("((p1 & p2) -> (a <-> b))", f1->pretty(false).c_str());
}

TEST(FormulaTest, ExpandExactlyNone) {
  m.game().declareVariables({"a1", "a2", "a3"});
  auto f1 = Formula::Parse("Exactly-0(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  ASSERT_TRUE(f2);
  auto f3 = f2->Expand();
  ASSERT_TRUE(f3);
  std::string s = f3->pretty(false);
  EXPECT_STREQ("((!a1) & (!a2) & (!a3))", s.c_str());
}

TEST(FormulaTest, ExpandExactlyAll) {
  m.game().declareVariables({"a1", "a2", "a3"});
  auto f1 = Formula::Parse("Exactly-3(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  ASSERT_TRUE(f2);
  auto f3 = f2->Expand();
  ASSERT_TRUE(f3);
  std::string s = f3->pretty(false);
  EXPECT_STREQ("((a1) & (a2) & (a3))", s.c_str());
}

TEST(SolverTest, BasicSatisfiability) {
  CnfFormula s;
  m.game().declareVariables({"a", "b", "c", "d"});
  s.AddConstraint(Formula::Parse("a -> b"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("c -> d"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("!b & !d"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("a & c"));
  s.InitSolver();
  EXPECT_FALSE(s.Satisfiable());
}

TEST(SolverTest, OtherSatisfiability) {
  CnfFormula s;
  m.game().declareVariables({"x", "a", "b", "c", "d"});
  s.AddConstraint(Formula::Parse("a&!b&!c&!d | !a&b&!c&!d | !a&!b&c&!d | !a&!b&!c&d"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("(x & a) | (!x & b)"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("(!x & b) | (x & c)"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
}

TEST(SolverTest, BasicFixed) {
  CnfFormula s;
  m.game().declareVariables({"x1", "x2", "x3", "x4", "x5"});
  s.AddConstraint(Formula::Parse("Exactly-2(x1, x2, x3, x4, x5)"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1, x2, x3)"));
  s.InitSolver();
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(s.GetFixedVariables(), 2); // x4 and x5 must be false.
}



