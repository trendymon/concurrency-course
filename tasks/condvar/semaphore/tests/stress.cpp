#include "../semaphore.hpp"
#include "../blocking_queue.hpp"

#include <twist/test/test.hpp>
#include <twist/test/runs.hpp>
#include <twist/test/inject_fault.hpp>

#include <twist/test/util/race.hpp>
#include <twist/test/util/latch.hpp>
#include <twist/test/util/barrier.hpp>

#include <wheels/test/util.hpp>

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

namespace pool {

class ResourcePool {
 public:
  ResourcePool(size_t limit) : limit_(limit), available_(limit) {
  }

  void Access() {
    ASSERT_TRUE_M(available_.fetch_sub(1) > 0, "Resource pool exhausted");
    twist::test::InjectFault();
    ASSERT_TRUE(available_.fetch_add(1) < (int)limit_);
  }

 private:
  size_t limit_;
  std::atomic<int> available_;
};

void Test(size_t threads, size_t limit) {
  ResourcePool pool{limit};
  solutions::Semaphore semaphore{limit};

  twist::test::util::Race race{threads};

  for (size_t t = 0; t < threads; ++t) {
    race.Add([&]() {
      while (wheels::test::KeepRunning()) {
        semaphore.Acquire();
        pool.Access();
        semaphore.Release();
      }
    });
  }

  race.Run();
}

}  // namespace pool

TWIST_TEST_RUNS(Pool, pool::Test)
    ->TimeLimit(7s)
    ->Run(5, 1)
    ->Run(5, 3)
    ->Run(10, 5)
    ->Run(10, 9);

////////////////////////////////////////////////////////////////////////////////

namespace wakeup {

void Test() {
  solutions::Semaphore semaphore{0};

  twist::test::util::CountDownLatch consumers_latch{2};
  twist::test::util::OnePassBarrier producers_barrier{2};

  auto consumer = [&]() {
    consumers_latch.CountDown();
    semaphore.Acquire();
  };

  auto producer = [&]() {
    producers_barrier.PassThrough();
    semaphore.Release();
  };

  twist::test::util::Race race{4};

  race.Add(consumer);
  race.Add(consumer);

  race.Add(producer);
  race.Add(producer);

  race.Run();
}

}  // namespace wakeup

TEST_SUITE(LostWakeup) {
  TWIST_ITERATE_TEST(Stress, 10s) {
    wakeup::Test();
  }
}

////////////////////////////////////////////////////////////////////////////////

namespace ping_pong {

void Test(const size_t iterations) {
  solutions::Semaphore left{1};
  solutions::Semaphore right{0};

  twist::test::util::Race race{2};

  race.Add([&]() {
    for (size_t i = 0; i < iterations; ++i) {
      right.Acquire();
      left.Release();
    }
  });

  race.Add([&]() {
    for (size_t i = 0; i < iterations; ++i) {
      left.Acquire();
      right.Release();
    }
  });

  race.Run();

  left.Acquire();
}

}  // namespace ping_pong

TEST_SUITE(PingPong) {
  TWIST_TEST_TL(Stress, 10s) {
    ping_pong::Test(10000);
  }
}

////////////////////////////////////////////////////////////////////////////////

namespace queue {

void Test(size_t producers, size_t consumers, size_t buffer_size) {
  static const std::string kPoisonPill = "";

  solutions::BlockingQueue<std::string> channel{buffer_size};

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};

  std::atomic<size_t> producers_left{producers};

  twist::test::util::Race race{producers + consumers};

  // Producers

  for (size_t p = 0; p < producers; ++p) {
    race.Add([&, p]() {
      int value = p;
      while (wheels::test::KeepRunning()) {
        channel.Put(std::to_string(value));
        produced.fetch_add(value);
        value += producers;
      }

      if (producers_left.fetch_sub(1) == 1) {
        // Last producer
        for (size_t c = 0; c < consumers; ++c) {
          channel.Put(kPoisonPill);
        }
      }
    });
  }

  // Consumers

  for (size_t c = 0; c < consumers; ++c) {
    race.Add([&]() {
      while (true) {
        std::string item = channel.Take();
        if (item.empty()) {
          break;
        }
        int value = std::stoi(item);
        consumed.fetch_add(value);
      }
    });
  }

  race.Run();

  ASSERT_EQ(produced.load(), consumed.load());
}

}  // namespace queue

TWIST_TEST_RUNS(BlockingQueue, queue::Test)
    ->TimeLimit(10s)
    ->Run(1, 1, 1)
    ->Run(5, 5, 16)
    ->Run(7, 2, 3)
    ->Run(3, 9, 4);

////////////////////////////////////////////////////////////////////////////////

RUN_ALL_TESTS()
