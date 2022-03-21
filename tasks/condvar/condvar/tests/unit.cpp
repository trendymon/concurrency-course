#include "../condvar.hpp"

#include <twist/test/test.hpp>

#include <twist/test/util/cpu_timer.hpp>

#include <twist/strand/stdlike.hpp>

#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

using twist::strand::stdlike::thread;
using twist::strand::stdlike::mutex;
using twist::strand::stdlike::this_thread::sleep_for;

TEST_SUITE(CondVar) {

  class Event {
   public:
    void Await() {
      std::unique_lock lock(mutex_);
      while (!set_) {
        set_cond_.Wait(lock);
      }
    }

    void Set() {
      std::lock_guard guard(mutex_);
      set_ = true;
      set_cond_.NotifyOne();
    }

    void Reset() {
      std::lock_guard guard(mutex_);
      set_ = false;
    }

   private:
    bool set_{false};
    mutex mutex_;
    stdlike::CondVar set_cond_;
  };

  SIMPLE_TWIST_TEST(NotifyOne) {
    Event pass;

    for (size_t i = 0; i < 3; ++i) {
      pass.Reset();

      bool passed = false;

      thread waiter([&]() {
          {
            twist::test::util::ThreadCPUTimer cpu_timer;
            pass.Await();
            ASSERT_TRUE(cpu_timer.Elapsed() < 200ms);
          }
          passed = true;
      });

      sleep_for(1s);

      ASSERT_FALSE(passed);

      pass.Set();
      waiter.join();

      ASSERT_TRUE(passed);
    }
  }

  class Latch {
   public:
    void Await() {
      std::unique_lock lock(mutex_);
      while (!released_) {
        released_cond_.Wait(lock);
      }
    }

    void Release() {
      std::lock_guard guard(mutex_);
      released_ = true;
      released_cond_.NotifyAll();
    }

    void Reset() {
      std::lock_guard guard(mutex_);
      released_ = false;
    }

   private:
    bool released_{false};
    mutex mutex_;
    stdlike::CondVar released_cond_;
  };

  SIMPLE_TWIST_TEST(NotifyAll) {
    Latch latch;

    for (size_t i = 0; i < 3; ++i) {
      latch.Reset();

      std::atomic<size_t> passed{0};

      auto wait_routine = [&]() {
        latch.Await();
        ++passed;
      };

      thread t1(wait_routine);
      thread t2(wait_routine);

      sleep_for(1s);

      ASSERT_EQ(passed.load(), 0);

      latch.Release();

      t1.join();
      t2.join();

      ASSERT_EQ(passed.load(), 2);
    }
  }

  SIMPLE_TWIST_TEST(NotifyManyTimes) {
    static const size_t kIterations = 1000'000;

    stdlike::CondVar cv;
    for (size_t i = 0; i < kIterations; ++i) {
      cv.NotifyOne();
    }
  }
}

RUN_ALL_TESTS()
