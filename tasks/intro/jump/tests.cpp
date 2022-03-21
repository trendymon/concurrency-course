#include <wheels/test/test_framework.hpp>

#include "jump.hpp"

#include <algorithm>
#include <random>
#include <vector>

TEST_SUITE(Jump) {

SIMPLE_TEST(Looping) {
  JumpContext ctx;

  size_t jump_count = 0;
  static const size_t kJumpsNeeded = 10;

  Capture(&ctx);

  ++jump_count;

  if (jump_count < kJumpsNeeded) {
    JumpTo(&ctx);  // Jump to Capture(&ctx)
    // JumpTo does not return
    FAIL_TEST("JumpTo returns control");
  }

  ASSERT_EQ(jump_count, kJumpsNeeded);
}

SIMPLE_TEST(PingPong) {
  JumpContext ping, pong;

  size_t count = 0;

  Capture(&ping);

  if (count == 42) {
    return;  // Test succeeded
  }

  if (count > 0) {
    JumpTo(&pong);
    FAIL_TEST("Unreachable");
  }

  ASSERT_TRUE(count == 0);

  Capture(&pong);
  ++count;
  JumpTo(&ping);

  FAIL_TEST("Unreachable");
}

SIMPLE_TEST(Canary) {
  struct Cage {
    JumpContext ctx;
    char canary = 42;
  } cage;

  bool jumped = false;

  Capture(&cage.ctx);
  if (!jumped) {
    jumped = true;
    JumpTo(&cage.ctx);
  }

  ASSERT_EQ(cage.canary, 42);
}

SIMPLE_TEST(TrashRegisters) {
  JumpContext ctx;
  int jumps = 0;

  Capture(&ctx);

  std::vector<int> ints;
  for (int i = 0; i < 1024; ++i) {
    ints.push_back(i);
  }

  std::random_device entropy;
  std::mt19937 twister(entropy());
  std::shuffle(ints.begin(), ints.end(), twister);

  std::sort(ints.begin(), ints.end());

  ++jumps;
  if (jumps < 10) {
    JumpTo(&ctx);
  }
}

}

RUN_ALL_TESTS()
