#include <wheels/test/test_framework.hpp>

#include <deep_thought.hpp>

#include <iostream>

TEST_SUITE(UltimateQuestion) {
  SIMPLE_TEST(ComputeAnswer) {
    solution::DeepThought deep_thought;
    int answer = deep_thought.ComputeAnswer();
    std::cout << "Your answer to the Ultimate Question of Life, the Universe, "
                 "and Everything is: "
              << answer << std::endl;
    ASSERT_EQ(answer, 42);
  }
}

RUN_ALL_TESTS()
