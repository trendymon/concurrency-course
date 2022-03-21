#include <mtf/fibers/api.hpp>

#include <wheels/test/test_framework.hpp>

#include <wheels/support/time.hpp>

using mtf::tp::StaticThreadPool;
using mtf::fibers::Spawn;
using mtf::fibers::Yield;

TEST_SUITE(Fibers) {
  SIMPLE_TEST(JustWorks) {
    StaticThreadPool pool{3};

    std::atomic<bool> done{false};

    auto tester = [&]() {
      ASSERT_EQ(mtf::tp::Current(), &pool);
      done.store(true);
    };

    Spawn(tester, pool);
    pool.Join();

    ASSERT_EQ(done.load(), true);
  }

  SIMPLE_TEST(Child) {
    StaticThreadPool pool{3};
    std::atomic<size_t> done{0};

    auto parent = [&]() {
      ASSERT_EQ(mtf::tp::Current(), &pool);

      auto child = [&]() {
        ASSERT_EQ(mtf::tp::Current(), &pool);
        ++done;
      };

      Spawn(child);

      ++done;
    };

    Spawn(parent, pool);

    pool.Join();

    ASSERT_EQ(done.load(), 2);
  }

  SIMPLE_TEST(RunInParallel) {
    StaticThreadPool pool{3};
    std::atomic<size_t> completed{0};

    auto sleeper = [&]() {
      std::this_thread::sleep_for(3s);
      completed.fetch_add(1);
    };

    wheels::StopWatch stop_watch;

    Spawn(sleeper, pool);
    Spawn(sleeper, pool);
    Spawn(sleeper, pool);

    pool.Join();

    ASSERT_EQ(completed.load(), 3);
    ASSERT_TRUE(stop_watch.Elapsed() < 3s + 500ms);
  }

  SIMPLE_TEST(Yield) {
    std::atomic<int> value{0};

    auto check_value = [&]() {
      const int kLimit = 10;

      ASSERT_TRUE(value.load() < kLimit);
      ASSERT_TRUE(value.load() > -kLimit);
    };

    static const size_t kIterations = 12345;

    auto bull = [&]() {
      for (size_t i = 0; i < kIterations; ++i) {
        value.fetch_add(1);
        Yield();
        check_value();
      }
    };

    auto bear = [&]() {
      for (size_t i = 0; i < kIterations; ++i) {
        value.fetch_sub(1);
        Yield();
        check_value();
      }
    };

    auto starter = [&]() {
      Spawn(bull);
      Spawn(bear);
    };

    StaticThreadPool pool{1};
    Spawn(starter, pool);
    pool.Join();
  }

  SIMPLE_TEST(Yield2) {
    StaticThreadPool pool{4};

    static const size_t kYields = 123456;

    auto tester = []() {
      for (size_t i = 0; i < kYields; ++i) {
        Yield();
      }
    };

    Spawn(tester, pool);
    Spawn(tester, pool);

    pool.Join();
  }

  class ForkTester {
   public:
    ForkTester(size_t threads) : pool_(threads) {
    }

    size_t Explode(size_t d) {
      Spawn(MakeForker(d), pool_);
      pool_.Join();
      return leafs_.load();
    }

   private:
    mtf::fibers::Routine MakeForker(size_t d) {
      return [this, d]() {
        if (d > 2) {
          Spawn(MakeForker(d - 2));
          Spawn(MakeForker(d - 1));
        } else {
          leafs_.fetch_add(1);
        }
      };
    }

   private:
    StaticThreadPool pool_;
    std::atomic<size_t> leafs_{0};
  };

  TEST(Forks, wheels::test::TestOptions().TimeLimit(10s).AdaptTLToSanitizer()) {
    ForkTester tester{4};
    // Respect ThreadSanitizer thread limit:
    // Tid - 13 bits => 8192 threads
    ASSERT_EQ(tester.Explode(20), 6765);
  }

  SIMPLE_TEST(TwoPools1) {
    StaticThreadPool pool_1{4};
    StaticThreadPool pool_2{4};

    auto make_tester = [](StaticThreadPool& pool) {
      return [&pool]() {
        ASSERT_EQ(mtf::tp::Current(), &pool);
      };
    };

    Spawn(make_tester(pool_1), pool_1);
    Spawn(make_tester(pool_2), pool_2);
  }

  SIMPLE_TEST(TwoPools2) {
    StaticThreadPool pool_1{4};
    StaticThreadPool pool_2{4};

    auto make_tester = [](StaticThreadPool& pool) {
      return [&pool]() {
        static const size_t kIterations = 1024;

        for (size_t i = 0; i < kIterations; ++i) {
          ASSERT_EQ(mtf::tp::Current(), &pool);

          Yield();

          Spawn([&pool]() {
            ASSERT_EQ(mtf::tp::Current(), &pool);
          });
        }
      };
    };

    Spawn(make_tester(pool_1), pool_1);
    Spawn(make_tester(pool_2), pool_2);
  }

  struct RacyCounter {
   public:
    void Increment() {
      value_.store(value_.load(std::memory_order_relaxed) + 1,
                   std::memory_order_relaxed);
    }
    size_t Get() const {
      return value_.load(std::memory_order_relaxed);
    }

   private:
    std::atomic<size_t> value_{0};
  };

  TEST(RacyCounter, wheels::test::TestOptions().TimeLimit(10s).AdaptTLToSanitizer()) {
    static const size_t kIncrements = 100'000;
    static const size_t kThreads = 4;
    static const size_t kFibers = 100;

    RacyCounter counter;

    auto routine = [&]() {
      for (size_t i = 0; i < kIncrements; ++i) {
        counter.Increment();
        if (i % 10 == 0) {
          Yield();
        }
      }
    };

    StaticThreadPool pool{kThreads};

    Spawn([&]() {
      for (size_t i = 0; i < kFibers; ++i) {
        Spawn(routine);
      }
    }, pool);

    pool.Join();

    std::cout << "Thread count: " << kThreads << std::endl
              << "Fibers: " << kFibers << std::endl
              << "Increments per fiber: " << kIncrements << std::endl
              << "Racy counter value: " << counter.Get() << std::endl;

    ASSERT_GE(counter.Get(), kIncrements);
    ASSERT_LT(counter.Get(), kIncrements * kFibers);
  }
}
