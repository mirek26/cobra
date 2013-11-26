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
  ASSERT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  ASSERT_STREQ(s.c_str(), "((!a1) & (!a2) & (!a3))");
}

TEST(FormulaTest, ExpandExactlyAll) {
  auto f1 = Formula::Parse("Exactly-3(a1, a2, a3)");
  auto f2 = dynamic_cast<ExactlyOperator*>(f1);
  ASSERT_TRUE(f1);
  auto f3 = f2->Expand();
  std::string s = f3->pretty(false);
  ASSERT_STREQ(s.c_str(), "((a1) & (a2) & (a3))");
}

TEST(SolverTest, BasicSatisfiability) {
  Solver s;
  s.AddConstraint(Formula::Parse("a -> b"));
  ASSERT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("c -> d"));
  ASSERT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("!b & !d"));
  ASSERT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("a & c"));
  ASSERT_FALSE(s.Satisfiable());
}

TEST(SolverTest, BasicFixed) {
  Solver s;
  s.AddConstraint(Formula::Parse("Exactly-2(x1..x5)"));
  ASSERT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1..x3)"));
  ASSERT_TRUE(s.Satisfiable());
  ASSERT_EQ(s.GetFixedVariables(NULL), 2); // x4 and x5 must be false.
}


