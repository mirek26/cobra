#include <vector>
#include <initializer_list>
#include "include/gtest/gtest.h"
#include "../src/formula.h"
#include "../src/pico-solver.h"
#include "../src/minisolver.h"
#include "../src/simple-solver.h"
#include "../src/parser.h"

extern Parser m;

// Parser tests.

TEST(Parser, UndefinedVariable) {
  m.reset();
  m.game().declareVars({"a", "b", "c"});
  EXPECT_NO_THROW(Formula::Parse("a & b & c"));
  EXPECT_THROW(Formula::Parse("a & b & c & d"), ParserException);
  EXPECT_THROW(Formula::Parse("Exactly-1(a1, a2, a3)"), ParserException);
}

TEST(Parser, BasicParse) {
  m.reset();
  m.game().declareVars({"p1", "p2", "a", "b"});
  auto f1 = Formula::Parse("p1 & p2 -> (a <-> b)");
  EXPECT_STREQ("((p1 & p2) -> (a <-> b))", f1->pretty(false).c_str());
}

// Tsetitin transformation tests.

TEST(Tseitin, Basic) {
  m.reset();
  m.game().declareVars({"x", "y"});
  auto f = Formula::Parse("!(And(Or(x&!y, y&!x)))");
  PicoSolver s1(m.game().vars(), f);
  // EXPECT_EQ(2, s1.NumOfModels());
  // PicoSolver s2(m.game().vars(), f);
  // EXPECT_EQ(2, s2.NumOfModels());
}

TEST(Tseitin, Exactly0) {
  m.reset();
  m.game().declareVars({"a1", "a2", "a3"});
  PicoSolver s(m.game().vars(),
               Formula::Parse("!((!a1 & !a2 & !a3) <-> Exactly-0(a1, a2, a3))"));
  EXPECT_FALSE(s.Satisfiable());
}

TEST(Tseitin, Exactly1) {
  m.reset();
  m.game().declareVars({"a1", "a2", "a3"});
  PicoSolver s(m.game().vars(),
               Formula::Parse("!((a1&!a2&!a3 | !a1&a2&!a3 | !a1&!a2&a3) <-> Exactly-1(a1, a2, a3))"));
  EXPECT_FALSE(s.Satisfiable());
}

TEST(Tseitin, Exactly1b) {
  m.reset();
  m.game().declareVars({"a", "b"});
  PicoSolver s(m.game().vars(),
               Formula::Parse("Exactly-1(a, a|b, b)"));
  EXPECT_FALSE(s.Satisfiable());
}

TEST(Tseitin, Exactly3) {
  m.reset();
  m.game().declareVars({"a1", "a2", "a3"});
  PicoSolver s(m.game().vars(),
               Formula::Parse("!((a1 & a2 & a3) <-> Exactly-3(a1, a2, a3))"));
  EXPECT_FALSE(s.Satisfiable());
}

TEST(SolverTest, Exactly1) {
  m.reset();
  m.game().declareVars({"a1", "a2", "a3"});
  PicoSolver s(m.game().vars(), Formula::Parse("Exactly-1(a1, a2, a3)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("a2 <-> a3"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("!a1"));
  EXPECT_FALSE(s.Satisfiable());
}

// Sat solver tests.

using testing::Types;
typedef Types<MiniSolver, PicoSolver, SimpleSolver> Implementations;
TYPED_TEST_CASE(SolverTest, Implementations);

template <class T>
class SolverTest : public testing::Test {
};

TYPED_TEST(SolverTest, BasicSatisfiability) {
  m.reset();
  m.game().declareVars({"a", "b", "c", "d"});
  TypeParam s(m.game().vars(), Formula::Parse("a -> b"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("c -> d"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("!b | !d"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("a & c"));
  EXPECT_FALSE(s.Satisfiable());
}

TYPED_TEST(SolverTest, OtherSatisfiability) {
  m.reset();
  m.game().declareVars({"x", "a", "b", "c", "d"});
  TypeParam s(m.game().vars(),
               Formula::Parse("a&!b&!c&!d | !a&b&!c&!d | !a&!b&c&!d | !a&!b&!c&d"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("(x & a) | (!x & b)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("(!x & b) | (x & c)"));
  EXPECT_TRUE(s.Satisfiable());
}

TYPED_TEST(SolverTest, GetAssignment) {
  m.reset();
  m.game().declareVars({"a", "b", "c"});
  TypeParam s(m.game().vars(), Formula::Parse("a & !b"));
  EXPECT_TRUE(s.Satisfiable());
  vec<bool> x = s.GetAssignment();
  EXPECT_EQ(4, x.size());
  EXPECT_TRUE(x[1]);
  EXPECT_FALSE(x[2]);
}

TYPED_TEST(SolverTest, MustBeTrueFalse) {
  m.reset();
  m.game().declareVars({"a", "b"});
  TypeParam s(m.game().vars(),
               Formula::Parse("(a -> b) & a"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_TRUE(s.MustBeTrue(1));
  EXPECT_FALSE(s.MustBeFalse(1));
  EXPECT_TRUE(s.MustBeTrue(2));
  EXPECT_FALSE(s.MustBeFalse(2));
}

TYPED_TEST(SolverTest, OnlyOneModel) {
  m.reset();
  m.game().declareVars({"a", "b"});
  TypeParam s(m.game().vars(),
               Formula::Parse("a -> b"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_FALSE(s.OnlyOneModel());
  s.AddConstraint(Formula::Parse("a"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_TRUE(s.OnlyOneModel());
}

TYPED_TEST(SolverTest, ExactlyFixed) {
  m.reset();
  m.game().declareVars({"x1", "x2", "x3", "x4", "x5"});
  TypeParam s(m.game().vars(),
               Formula::Parse("Exactly-2(x1, x2, x3, x4, x5)"));
  EXPECT_TRUE(s.Satisfiable());
  s.AddConstraint(Formula::Parse("AtLeast-2(x1, x2, x3)"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(2, s.GetNumOfFixedVars()); // x4 and x5 must be false.
}

TYPED_TEST(SolverTest, NumOfModelsExactly2) {
  m.reset();
  m.game().declareVars({"x1", "x2", "x3", "x4", "x5"});
  TypeParam s(m.game().vars(),
               Formula::Parse("Exactly-2(x1, x2, x3, x4, x5)"));
  EXPECT_EQ(10, s.NumOfModels()); // 5 choose 2
}

TYPED_TEST(SolverTest, NumOfModelsAtMost2) {
  m.reset();
  m.game().declareVars({"x1", "x2", "x3", "x4", "x5"});
  TypeParam s(m.game().vars(),
               Formula::Parse("AtMost-2(x1, x2, x3, x4, x5)"));
  EXPECT_EQ(16, s.NumOfModels()); // 1 + 5 + 10
}


TYPED_TEST(SolverTest, NumOfModelsAtLeast2) {
  m.reset();
  m.game().declareVars({"x1", "x2", "x3", "x4", "x5"});
  TypeParam s(m.game().vars(),
               Formula::Parse("AtLeast-2(x1, x2, x3, x4, x5)"));
  EXPECT_EQ(26, s.NumOfModels()); // 2^5 - 5 - 1
}

// TYPED_TEST(SolverTest, NumOfModelsSharpSat) {
//   m.reset();
//   m.game().declareVars({"x1", "x2", "x3", "x4", "x5"});
//   TypeParam s(m.game().vars(),
//                Formula::Parse("Exactly-2(x1, x2, x3, x4, x5)"));
//   EXPECT_EQ(10, s.NumOfModelsSharpSat());
// }

// NumOfModelsUnsat
// NumOfModelsSharpSatUnsat

TYPED_TEST(SolverTest, Context) {
  m.reset();
  m.game().declareVars({"a", "b", "c", "d"});
  TypeParam s(m.game().vars(),
               Formula::Parse("(a -> b) & (c -> d) & (!b | !d)"));
  EXPECT_TRUE(s.Satisfiable());
  EXPECT_EQ(5, s.NumOfModels());
  s.OpenContext();
  s.AddConstraint(Formula::Parse("a & c"));
  EXPECT_FALSE(s.Satisfiable());
  s.CloseContext();
  EXPECT_EQ(5, s.NumOfModels());
}

TYPED_TEST(SolverTest, NestedContext) {
  m.reset();
  m.game().declareVars({"a", "b", "c", "d"});
  TypeParam s(m.game().vars(),
               Formula::Parse("a | b"));
  // EXPECT_EQ("(a | b)", s.pretty());
  EXPECT_EQ(12, s.NumOfModels()); // a|b -> 3 * 2^2
  s.OpenContext();
  s.AddConstraint(Formula::Parse("c | d"));
  // EXPECT_EQ("(a | b) & (c | d)", s.pretty());
  EXPECT_EQ(9, s.NumOfModels()); // (a|b) & (c|d) -> 3*3
  s.OpenContext();
  s.AddConstraint(Formula::Parse("a | d"));
  // EXPECT_EQ("(a | b) & (c | d) & (a | d)", s.pretty());
  EXPECT_EQ(8, s.NumOfModels()); // (a|b) & (c|d) & (a|d) -> as before - 0110
  s.CloseContext();
  // EXPECT_EQ("(a | b) & (c | d)", s.pretty());
  EXPECT_EQ(9, s.NumOfModels()); // (a|b) & (c|d)
  s.AddConstraint(Formula::Parse("!a"));
  //EXPECT_EQ("(a | b) & (c | d) & (-a)", s.pretty());
  EXPECT_EQ(3, s.NumOfModels()); // !a & (a|b) & (c|d) -> 3
  s.CloseContext();
  // EXPECT_EQ("(a | b)", s.pretty());
  EXPECT_EQ(12, s.NumOfModels()); // a|b
}


