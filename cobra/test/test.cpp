#include <vector>
#include "include/gtest/gtest.h"
#include "../formula.h"
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

