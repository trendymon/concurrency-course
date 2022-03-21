#include "../mutex.hpp"

#include <twist/test/test.hpp>

#include <twist/test/util/cpu_timer.hpp>
#include <twist/strand/stdlike/thread.hpp>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

using stdlike::Mutex;

using twist::strand::stdlike::thread;
using twist::strand::stdlike::this_thread::sleep_for;

TEST_SUITE(UnitTest) {
  SIMPLE_TWIST_TEST(LockUnlock) {
    Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
  }

SIMPLE_TWIST_TEST(SequentialLockUnlock) {
    Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
    mutex.Lock();
    mutex.Unlock();
  }

  SIMPLE_TWIST_TEST(NoSharedLocations) {
    Mutex mutex;
    mutex.Lock();

    Mutex mutex2;
    mutex2.Lock();
    mutex2.Unlock();

    mutex.Unlock();
  }

  SIMPLE_TWIST_TEST(MutualExclusion) {
    Mutex mutex;
    bool cs = false;

    thread locker([&]() {
      mutex.Lock();
      cs = true;
      sleep_for(3s);
      cs = false;
      mutex.Unlock();
    });

    sleep_for(1s);
    mutex.Lock();
    ASSERT_FALSE(cs);
    mutex.Unlock();

    locker.join();
  }

#if !defined(TWIST_FIBER) && !defined(TWIST_FAULTY)

  SIMPLE_TWIST_TEST(Blocking) {
    Mutex mutex;

    // Warmup
    mutex.Lock();
    mutex.Unlock();

    thread sleeper([&]() {
      mutex.Lock();
      sleep_for(3s);
      mutex.Unlock();
    });

    thread waiter([&]() {
      sleep_for(1s);

      twist::test::util::ThreadCPUTimer cpu_timer;

      mutex.Lock();
      mutex.Unlock();

      auto elapsed = cpu_timer.Elapsed();

      ASSERT_TRUE(elapsed < 200ms);
    });

    sleeper.join();
    waiter.join();
  }

#endif
}

RUN_ALL_TESTS()
