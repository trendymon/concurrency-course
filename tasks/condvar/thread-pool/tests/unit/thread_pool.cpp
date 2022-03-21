#include <tp/static_thread_pool.hpp>

#include <wheels/test/test_framework.hpp>

#include <wheels/support/cpu_time.hpp>
#include <wheels/support/time.hpp>

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_SUITE(ThreadPool) {
  SIMPLE_TEST(JustWorks) {
    tp::StaticThreadPool pool{4};

    pool.Submit([]() {
      std::cout << "Hello from thread pool!" << std::endl;
    });

    pool.Join();
  }

  SIMPLE_TEST(Join) {
    tp::StaticThreadPool pool{4};

    bool done = false;

    pool.Submit([&]() {
      std::this_thread::sleep_for(1s);
      done = true;
    });

    pool.Join();

    ASSERT_TRUE(done);
  }

  SIMPLE_TEST(Exceptions) {
    tp::StaticThreadPool pool{1};

    pool.Submit([]() {
      throw std::runtime_error("Task failed");
    });

    pool.Join();
  }

  SIMPLE_TEST(ManyTasks) {
    tp::StaticThreadPool pool{4};

    static const size_t kTasks = 17;

    std::atomic<size_t> tasks{0};

    for (size_t i = 0; i < kTasks; ++i) {
      pool.Submit([&]() {
        ++tasks;
      });
    }

    pool.Join();

    ASSERT_EQ(tasks.load(), kTasks);
  }

  SIMPLE_TEST(Parallel) {
    tp::StaticThreadPool pool{4};

    std::atomic<size_t> tasks{0};

    pool.Submit([&]() {
      std::this_thread::sleep_for(1s);
      ++tasks;
    });

    pool.Submit([&]() {
      ++tasks;
    });

    std::this_thread::sleep_for(100ms);

    ASSERT_EQ(tasks.load(), 1);

    pool.Join();

    ASSERT_EQ(tasks.load(), 2);
  }

  SIMPLE_TEST(TwoPools) {
    tp::StaticThreadPool pool1{1};
    tp::StaticThreadPool pool2{1};

    std::atomic<size_t> tasks{0};

    wheels::StopWatch stop_watch;

    pool1.Submit([&]() {
      std::this_thread::sleep_for(1s);
      ++tasks;
    });

    pool2.Submit([&]() {
      std::this_thread::sleep_for(1s);
      ++tasks;
    });

    pool2.Join();
    pool1.Join();

    ASSERT_TRUE(stop_watch.Elapsed() < 1500ms);
    ASSERT_EQ(tasks.load(), 2);
  }

  SIMPLE_TEST(Shutdown) {
    tp::StaticThreadPool pool{3};

    for (size_t i = 0; i < 3; ++i) {
      pool.Submit([]() {
        std::this_thread::sleep_for(1s);
      });
    }

    for (size_t i = 0; i < 10; ++i) {
      pool.Submit([]() {
        std::this_thread::sleep_for(100s);
      });
    }

    std::this_thread::sleep_for(250ms);

    pool.Shutdown();
  }

  SIMPLE_TEST(DoNotBurnCPU) {
    tp::StaticThreadPool pool{4};

    // Warmup
    for (size_t i = 0; i < 4; ++i) {
      pool.Submit([&]() {
        std::this_thread::sleep_for(100ms);
      });
    }

    wheels::ProcessCPUTimer cpu_timer;

    std::this_thread::sleep_for(1s);

    pool.Join();

    ASSERT_TRUE(cpu_timer.Elapsed() < 100ms);
  }

  SIMPLE_TEST(Current) {
    tp::StaticThreadPool pool{1};

    ASSERT_EQ(tp::Current(), nullptr);

    pool.Submit([&]() {
      ASSERT_EQ(tp::Current(), &pool);
    });

    pool.Join();
  }

  SIMPLE_TEST(SubmitAfterJoin) {
    tp::StaticThreadPool pool{4};

    bool done = false;

    pool.Submit([&]() {
      std::this_thread::sleep_for(500ms);
      tp::Current()->Submit([&]() {
        std::this_thread::sleep_for(500ms);
        done = true;
      });
    });

    pool.Join();

    ASSERT_TRUE(done);
  }

  SIMPLE_TEST(SubmitAfterShutdown) {
    tp::StaticThreadPool pool{4};

    bool done = false;

    pool.Submit([&]() {
      std::this_thread::sleep_for(500ms);
      tp::Current()->Submit([&]() {
        std::this_thread::sleep_for(500ms);
        done = true;
      });
    });

    pool.Shutdown();

    ASSERT_FALSE(done);
  }

  TEST(UseThreads, wheels::test::TestOptions().TimeLimit(1s)) {
    tp::StaticThreadPool pool{4};

    std::atomic<size_t> tasks{0};

    for (size_t i = 0; i < 4; ++i) {
      pool.Submit([&]() {
        std::this_thread::sleep_for(750ms);
        ++tasks;
      });
    }

    pool.Join();

    ASSERT_EQ(tasks.load(), 4);
  }

  TEST(TooManyThreads, wheels::test::TestOptions().TimeLimit(2s)) {
    tp::StaticThreadPool pool{3};

    std::atomic<size_t> tasks{0};

    for (size_t i = 0; i < 4; ++i) {
      pool.Submit([&]() {
        std::this_thread::sleep_for(750ms);
        ++tasks;
      });
    }

    wheels::StopWatch stop_watch;

    pool.Join();

    ASSERT_TRUE(stop_watch.Elapsed() > 1s);
    ASSERT_EQ(tasks.load(), 4);
  }

  void KeepAlive() {
    if (wheels::test::TestTimeLeft() > 300ms) {
      tp::Current()->Submit([]() {
        KeepAlive();
      });
    }
  }

  TEST(KeepAlive, wheels::test::TestOptions().TimeLimit(4s)) {
    tp::StaticThreadPool pool{3};

    for (size_t i = 0; i < 5; ++i) {
      pool.Submit([]() {
        KeepAlive();
      });
    }

    wheels::StopWatch stop_watch;

    pool.Join();

    ASSERT_TRUE(stop_watch.Elapsed() > 3s);
  }

  SIMPLE_TEST(TaskLifetime) {
    tp::StaticThreadPool pool{4};

    std::atomic<int> dead{0};

    class Task {
     public:
      Task(std::atomic<int>& done) : counter_(done) {
      }
      Task(const Task&) = delete;
      Task(Task&&) = default;

      ~Task() {
        if (done_) {
          counter_.fetch_add(1);
        }
      }

      void operator()() {
        std::this_thread::sleep_for(100ms);
        done_ = true;
      }

     private:
      bool done_{false};
      std::atomic<int>& counter_;
    };

    for (int i = 0; i < 4; ++i) {
      pool.Submit(Task(dead));
    }
    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(dead.load(), 4)
    pool.Join();
  }

  SIMPLE_TEST(Racy) {
    tp::StaticThreadPool pool{4};

    std::atomic<int> shared_counter{0};
    std::atomic<int> tasks{0};

    for (size_t i = 0; i < 100500; ++i) {
      pool.Submit([&]() {
        int old = shared_counter.load();
        shared_counter.store(old + 1);

        ++tasks;
      });
    }

    pool.Join();

    std::cout << "Racy counter value: " << shared_counter << std::endl;

    ASSERT_EQ(tasks.load(), 100500);
    ASSERT_LE(shared_counter.load(), 100500);
  }
}
