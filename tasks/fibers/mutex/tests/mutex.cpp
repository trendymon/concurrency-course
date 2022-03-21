#include <mtf/fibers/core/api.hpp>
#include <mtf/fibers/sync/mutex.hpp>

#include <twist/test/test.hpp>
#include <twist/test/util/plate.hpp>

#include <wheels/test/util.hpp>

#include <wheels/support/cpu_time.hpp>

using namespace mtf::fibers;
using mtf::tp::StaticThreadPool;

using namespace std::chrono_literals;

TEST_SUITE(Mutex) {
  SIMPLE_TEST(JustWorks) {
    StaticThreadPool pool{4};

    Mutex mutex;

    Spawn(pool, [&mutex]() {
      mutex.Lock();
      mutex.Unlock();

      mutex.Lock();
      mutex.Unlock();
    });

    pool.Join();
  }

  SIMPLE_TEST(DoNotBlockPoolThread) {
    StaticThreadPool pool{2};

    Mutex mutex;

    size_t cs_count = 0;

    Spawn(pool, [&]() {
      mutex.Lock();
      std::this_thread::sleep_for(1s);
      ++cs_count;
      mutex.Unlock();
    });

    for (size_t i = 0; i < 3; ++i) {
      Spawn(pool, [&]() {
        mutex.Lock();
        ++cs_count;
        mutex.Unlock();
      });
    }

    std::this_thread::sleep_for(100ms);

    std::atomic<bool> free{false};

    pool.Submit([&free]() {
      free.store(true);
    });

    std::this_thread::sleep_for(100ms);

    ASSERT_TRUE(free.load());

    pool.Join();

    ASSERT_EQ(cs_count, 4);
  }

#if !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)

  SIMPLE_TEST(BlockFiber) {
    wheels::ProcessCPUTimer cpu_timer;

    StaticThreadPool pool{4};

    Mutex mutex;

    Spawn(pool, [&mutex]() {
      mutex.Lock();
      std::this_thread::sleep_for(1s);
      mutex.Unlock();
    });

    std::this_thread::sleep_for(100ms);

    size_t cs_count{0};

    for (size_t i = 0; i < 10; ++i) {
      Spawn(pool, [&]() {
        mutex.Lock();
        ++cs_count;
        mutex.Unlock();
      });
    }

    pool.Join();

    ASSERT_TRUE(cpu_timer.Elapsed() < 100ms);
    ASSERT_EQ(cs_count, 10);
  }

#endif

  void MutexStressTest(size_t threads, size_t fibers) {
    twist::test::util::Plate plate;  // Guarded by mutex
    Mutex mutex;

    StaticThreadPool scheduler{threads};

    for (size_t i = 0; i < fibers; ++i) {
      Spawn(scheduler, [&]() {
        while (wheels::test::KeepRunning()) {
          mutex.Lock();
          {
            // Critical section
            plate.Access();
          }
          mutex.Unlock();
        }
      });
    }

    scheduler.Join();

    std::cout << "Critical sections: " << plate.AccessCount() << std::endl;

    ASSERT_TRUE(plate.AccessCount() > 12345);
  }

  TWIST_TEST_TL(Stress1, 5s) {
    MutexStressTest(/*threads=*/4, /*fibers=*/2);
  }

  TWIST_TEST_TL(Stress2, 5s) {
    MutexStressTest(/*threads=*/2, /*fibers=*/100);
  }

  TWIST_TEST_TL(Stress3, 5s) {
    MutexStressTest(/*threads=*/4, /*fibers=*/10);
  }

  TWIST_TEST_TL(Stress4, 5s) {
    MutexStressTest(/*threads=*/5, /*fibers=*/300);
  }

  void MissedWakeupTest(size_t threads, size_t fibers) {
    StaticThreadPool pool{threads};

    Mutex mutex;

    for (size_t i = 0; i < fibers; ++i) {
      Spawn(pool, [&mutex]() {
        mutex.Lock();
        mutex.Unlock();
      });
    };

    pool.Join();
  }

  TWIST_ITERATE_TEST(WakeupStress1, 5s) {
    MissedWakeupTest(/*threads=*/2, /*fibers=*/2);
  }

  TWIST_ITERATE_TEST(WakeupStress2, 5s) {
    MissedWakeupTest(/*threads=*/2, /*fibers=*/3);
  }
}
