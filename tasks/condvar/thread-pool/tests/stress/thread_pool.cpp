#include <tp/static_thread_pool.hpp>

#include <twist/test/test.hpp>
#include <twist/test/runs.hpp>
#include <twist/test/util/race.hpp>

#include <twist/stdlike/atomic.hpp>

#include <atomic>

////////////////////////////////////////////////////////////////////////////////

namespace tasks {

void KeepAlive() {
  if (twist::test::KeepRunning()) {
    tp::Current()->Submit([]() {
      KeepAlive();
    });
  }
}

void Backoff() {
  twist::strand::stdlike::this_thread::yield();
}

void Test(size_t threads, size_t clients, size_t limit) {
  tp::StaticThreadPool pool{threads};

  pool.Submit([]() {
    KeepAlive();
  });

  std::atomic<size_t> completed{0};

  twist::stdlike::atomic<size_t> queue{0};

  twist::test::util::Race race;

  for (size_t i = 0; i < clients; ++i) {
    race.Add([&]() {
      while (twist::test::KeepRunning()) {
        // TrySubmit
        if (++queue <= limit) {
          pool.Submit([&]() {
            --queue;
            ++completed;
          });
        } else {
          --queue;
          Backoff();
        }
      }
    });
  }

  race.Run();

  pool.Join();

  std::cout << "Tasks completed: " << completed.load() << std::endl;

  ASSERT_EQ(queue.load(), 0);
  ASSERT_GT(completed.load(), 8888);
}

}  // namespace tasks

TWIST_TEST_RUNS(Submits, tasks::Test)
  ->TimeLimit(4s)
  ->Run(3, 5, 111)
  ->Run(4, 3, 13)
  ->Run(2, 4, 5)
  ->Run(9, 10, 33);

////////////////////////////////////////////////////////////////////////////////

namespace join {

void TestSequential() {
  tp::StaticThreadPool pool{4};

  std::atomic<size_t> tasks{0};

  pool.Submit([&]() {
    ++tasks;
  });
  pool.Join();

  ASSERT_EQ(tasks.load(), 1);
}

void TestConcurrent() {
  tp::StaticThreadPool pool{2};

  std::atomic<size_t> tasks{0};

  twist::test::util::Race race;

  race.Add([&]() {
    pool.Submit([&]() {
      ++tasks;
    });
  });

  race.Add([&]() {
    pool.Join();
  });

  race.Run();

  ASSERT_LE(tasks.load(), 1);
}

void TestCurrent() {
  tp::StaticThreadPool pool{2};

  std::atomic<bool> done{false};

  pool.Submit([&]() {
    tp::Current()->Submit([&]() {
      done = true;
    });
  });
  pool.Join();

  ASSERT_TRUE(done.load());
}

}  // namespace join

TEST_SUITE(Join) {
  TWIST_ITERATE_TEST(Sequential, 5s) {
    join::TestSequential();
  }

  TWIST_ITERATE_TEST(Concurrent, 5s) {
    join::TestConcurrent();
  }

  TWIST_ITERATE_TEST(Current, 5s) {
    join::TestCurrent();
  }
}
