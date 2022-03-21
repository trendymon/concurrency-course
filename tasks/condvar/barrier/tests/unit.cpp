#include "../cyclic_barrier.hpp"

#include <twist/test/test.hpp>

#include <twist/strand/stdlike.hpp>

using twist::strand::stdlike::thread;

TEST_SUITE(CyclicBarrier) {
  SIMPLE_TWIST_TEST(OneThread) {
    solutions::CyclicBarrier barrier{1};

    for (size_t i = 0; i < 10; ++i) {
      barrier.Arrive();
    }
  }

  SIMPLE_TWIST_TEST(TwoThreads) {
    solutions::CyclicBarrier barrier{2};

    int my = 0;
    int that = 0;

    thread that_thread([&]() {
      that = 1;
      barrier.Arrive();
      ASSERT_EQ(my, 1);
      barrier.Arrive();
      that = 2;
      barrier.Arrive();
      ASSERT_EQ(my, 2);
    });

    my = 1;
    barrier.Arrive();
    ASSERT_EQ(that, 1);
    barrier.Arrive();
    my = 2;
    barrier.Arrive();
    ASSERT_EQ(that, 2);

    that_thread.join();
  }

  SIMPLE_TWIST_TEST(Runners) {
    static const size_t kThreads = 10;
    solutions::CyclicBarrier barrier{kThreads};

    static const size_t kIterations = 256;

    std::vector<thread> runners;

    for (size_t i = 0; i < kThreads; ++i) {
      runners.emplace_back([&barrier]() {
        for (size_t i = 0; i < kIterations; ++i) {
          barrier.Arrive();
        }
      });
    }

    for (auto& runner : runners) {
      runner.join();
    }
  }
}

RUN_ALL_TESTS()
