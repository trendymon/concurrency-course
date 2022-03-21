#include "../semaphore.hpp"
#include "blocking_queue.hpp"

#include <twist/test/test.hpp>
#include <twist/test/random.hpp>

#include <twist/strand/stdlike.hpp>

#include <atomic>
#include <deque>
#include <chrono>
#include <string>

////////////////////////////////////////////////////////////////////////////////

using twist::strand::stdlike::thread;
using twist::strand::stdlike::this_thread::sleep_for;

////////////////////////////////////////////////////////////////////////////////

TEST_SUITE(Semaphore) {
  SIMPLE_TWIST_TEST(NonBlocking) {
    solutions::Semaphore semaphore(2);

    semaphore.Acquire();  // -1
    semaphore.Release();  // +1

    semaphore.Acquire();  // -1
    semaphore.Acquire();  // -1
    semaphore.Release();  // +1
    semaphore.Release();  // +1
  }

  SIMPLE_TWIST_TEST(Blocking) {
    solutions::Semaphore semaphore(0);

    bool touched = false;

    thread touch([&]() {
      semaphore.Acquire();
      touched = true;
    });

    sleep_for(250ms);

    ASSERT_FALSE(touched);

    semaphore.Release();
    touch.join();

    ASSERT_TRUE(touched);
  }

  SIMPLE_TWIST_TEST(PingPong) {
    solutions::Semaphore my{1};
    solutions::Semaphore that{0};

    int step = 0;

    thread opponent([&]() {
      that.Acquire();
      ASSERT_EQ(step, 1);
      step = 0;
      my.Release();
    });

    my.Acquire();
    ASSERT_EQ(step, 0);
    step = 1;
    that.Release();

    my.Acquire();
    ASSERT_TRUE(step == 0);

    opponent.join();
  }
}

////////////////////////////////////////////////////////////////////////////////

TEST_SUITE(BlockingQueue) {
  SIMPLE_TWIST_TEST(PutThenTake) {
    solutions::BlockingQueue<int> queue{1};
    queue.Put(42);
    ASSERT_EQ(queue.Take(), 42);
  }

  struct MoveOnly {
    MoveOnly() = default;

    MoveOnly(const MoveOnly& that) = delete;
    MoveOnly& operator=(const MoveOnly& that) = delete;

    MoveOnly(MoveOnly&& that) = default;
    MoveOnly& operator=(MoveOnly&& that) = default;
  };

  SIMPLE_TWIST_TEST(MoveOnly) {
    solutions::BlockingQueue<MoveOnly> queue{1};

    queue.Put(MoveOnly{});
    queue.Take();
  }

  SIMPLE_TWIST_TEST(Buffer) {
    solutions::BlockingQueue<std::string> queue{2};

    queue.Put("hello");
    queue.Put("world");

    ASSERT_EQ(queue.Take(), "hello");
    ASSERT_EQ(queue.Take(), "world");
  }

  SIMPLE_TWIST_TEST(FifoSmall) {
    solutions::BlockingQueue<std::string> queue{2};

    thread producer([&queue]() {
      queue.Put("hello");
      queue.Put("world");
      queue.Put("!");
    });

    ASSERT_EQ(queue.Take(), "hello");
    ASSERT_EQ(queue.Take(), "world");
    ASSERT_EQ(queue.Take(), "!");

    producer.join();
  }

  SIMPLE_TWIST_TEST(Fifo) {
    solutions::BlockingQueue<int> queue{3};

    static const int kItems = 1024;

    thread producer([&]() {
      for (int i = 0; i < kItems; ++i) {
        queue.Put(i);
      }
      queue.Put(-1);  // Poison pill
    });

    // Consumer

    for (int i = 0; i < kItems; ++i) {
      ASSERT_EQ(queue.Take(), i);
    }
    ASSERT_EQ(queue.Take(), -1);

    producer.join();
  }

  SIMPLE_TWIST_TEST(Capacity) {
    solutions::BlockingQueue<int> queue{3};
    std::atomic<size_t> send_count{0};

    thread producer([&]() {
      for (size_t i = 0; i < 100; ++i) {
        queue.Put(i);
        send_count.store(i);
      }
      queue.Put(-1);
    });

    sleep_for(100ms);

    ASSERT_TRUE(send_count.load() <= 3);

    for (size_t i = 0; i < 14; ++i) {
      (void)queue.Take();
    }

    sleep_for(100ms);

    ASSERT_TRUE(send_count.load() <= 17);

    while (queue.Take() != -1) {
      // Pass
    }

    producer.join();
  }

  SIMPLE_TWIST_TEST(Pill) {
    static const size_t kThreads = 10;
    solutions::BlockingQueue<int> queue{1};

    std::vector<thread> threads;

    for (size_t i = 0; i < kThreads; ++i) {
      threads.emplace_back([&]() {
        sleep_for(std::chrono::milliseconds(
            twist::test::RandomUInteger(1000)));

        ASSERT_EQ(queue.Take(), -1);
        queue.Put(-1);
      });
    }

    queue.Put(-1);

    for (auto& t : threads) {
      t.join();
    }
  }
}

RUN_ALL_TESTS()
