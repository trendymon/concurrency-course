#include <wheels/test/test_framework.hpp>

#include <tinyfibers/api.hpp>
#include <tinyfibers/runtime/scheduler.hpp>
#include <tinyfibers/sync/wait_group.hpp>

#include <wheels/support/quick_exit.hpp>

using tinyfibers::Scheduler;
using tinyfibers::WaitGroup;
using tinyfibers::self::Yield;

TEST_SUITE(TrickyLock) {
  // TrickyLock example for cooperative fibers
  TEST(LiveLock, wheels::test::TestOptions().ForceFork()) {
    Scheduler scheduler;

    auto test = []() {
      static const size_t kIterations = 100;

      size_t cs_count = 0;

      // TrickyLock state
      size_t thread_count = 0;

      auto contender = [&]() {
        // Put Yield-s to produce livelock

        for (size_t i = 0; i < kIterations; ++i) {
          // TrickyLock::Lock
          while (thread_count++ > 0) { // f1: 0 -> 1, f2: 1 -> 2, f1: 1 -> 2, f2: 1 -> 2, f1: 1 -> 2...
            Yield(); // f2: 2, f1: 2, f2: 2...
            --thread_count; // f2: 2 -> 1, f1: 2 -> 1...
          }
          // Spinlock acquired
          {
            // Critical section
            ++cs_count;
            ASSERT_TRUE_M(cs_count < 3, "Too many critical sections")
            // End of critical section
          }

          Yield();  // f1: 1
          // TrickyLock::Unlock
          --thread_count; // f1: 2 -> 1
          // Spinlock released
        }
      };

      // Spawn two fibers
      WaitGroup wg;
      wg.Spawn(contender).Spawn(contender).Wait();
    };

    // Limit number of scheduler run loop iterations
    scheduler.Run(test, /*fuel=*/123456);

    // World is broken, leave it ASAP
    wheels::QuickExit(0);
  }
}

RUN_ALL_TESTS()
