#include <mtf/fibers/core/api.hpp>
#include <mtf/fibers/sync/channel.hpp>
#include <mtf/fibers/sync/select.hpp>
#include <mtf/fibers/test/test.hpp>

#include <twist/test/test.hpp>

#include <wheels/test/test_framework.hpp>
#include <wheels/test/util.hpp>

#include <wheels/support/cpu_time.hpp>

#include <chrono>
#include <string>

using mtf::fibers::Channel;
using mtf::fibers::Select;
using mtf::fibers::Spawn;
using mtf::fibers::Yield;
using mtf::tp::StaticThreadPool;

using namespace std::chrono_literals;

//////////////////////////////////////////////////////////////////////

struct Tag1 {};
struct Tag2 {};

template <typename Tag>
struct MoveOnly {
  MoveOnly() = delete;

  MoveOnly(std::string _data) : data(std::move(_data)) {
  }

  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;

  // Non-copyable
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  std::string data;
};

//////////////////////////////////////////////////////////////////////

static const auto kLongTestOptions = wheels::test::TestOptions().TimeLimit(60s);

//////////////////////////////////////////////////////////////////////

TEST_SUITE(Channels) {
  SIMPLE_FIBER_TEST(JustWorks, 1) {
    Channel<int> ints{5};
    ints.Send(1);
    ints.Send(2);
    ints.Send(3);
    ASSERT_EQ(ints.Receive(), 1);
    ASSERT_EQ(ints.Receive(), 2);
    ASSERT_EQ(ints.Receive(), 3);
  }

#if !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)

  SIMPLE_TEST(Blocking1) {
    StaticThreadPool scheduler{2};

    wheels::ProcessCPUTimer cpu_timer;

    Channel<int> ints{5};

    std::atomic<bool> done{false};

    Spawn(scheduler, [&]() {
      ints.Receive();
      done = true;
    });

    std::this_thread::sleep_for(1s);
    ASSERT_FALSE(done.load());

    Spawn(scheduler, [&]() {
      ints.Send(7);
    });

    scheduler.Join();

    ASSERT_TRUE(cpu_timer.Elapsed() < 50ms);
    ASSERT_TRUE(done.load());
  }

#endif

  SIMPLE_FIBER_TEST(MoveOnly, 1) {
    Channel<MoveOnly<Tag1>> messages{5};

    messages.Send({"Hello"});
    MoveOnly message = messages.Receive();
    ASSERT_EQ(message.data, "Hello");
  }

  SIMPLE_TEST(BlockSenders) {
    StaticThreadPool scheduler{1};

    static const size_t kBufferSize = 17;

    Channel<int> ints{kBufferSize};

    std::atomic<bool> enough{false};
    size_t received = 0;

    // Producer
    Spawn(scheduler, [&]() {
      while (!enough.load()) {
        ints.Send(7);
      }
      ints.Send(-1);
    });

    // Consumer
    Spawn(scheduler, [&]() {
      for (size_t i = 0; i < 7; ++i) {
        Yield();
      }

      // Producer blocked
      enough.store(true);

      while (true) {
        int value = ints.Receive();
        if (value == -1) {
          // Poison pill
          break;
        } else {
          ++received;
        }
      }
    });

    scheduler.Join();

    ASSERT_EQ(received, kBufferSize + 1);
  }

  SIMPLE_TEST(WakeSenders) {
    StaticThreadPool scheduler{1};

    bool done = false;
    Channel<int> ints{1};

    Spawn(scheduler, [&] {
      ints.Send(1);
      ints.Send(2);  // Blocked
      done = true;
    });

    Spawn(scheduler, [&] {
      ASSERT_EQ(ints.Receive(), 1);
    });

    scheduler.Join();
    ASSERT_TRUE(done);
  }

  TEST(ConcurrentReceivers, kLongTestOptions) {
    Channel<int64_t> ints{7};

    StaticThreadPool pool{4};

    static const size_t kFibers = 12;

    std::atomic<int64_t> received{0};

    // Receivers

    for (size_t i = 0; i < kFibers; ++i) {
      Spawn(pool, [&]() {
        while (true) {
          int value = ints.Receive();
          if (value == -1) {
            break;
          }
          received.fetch_add(value);
        }
      });
    }

    // Sender

    static const int kItems = 100000;

    int64_t sent = 0;

    Spawn(pool, [&]() {
      for (int i = 0; i < kItems; ++i) {
        ints.Send(i);
        sent += i;
      }

      // Poison pills
      for (size_t i = 0; i < kFibers; ++i) {
        ints.Send(-1);
      }
    });

    pool.Join();

    ASSERT_EQ(sent, received.load());
  }

  void TestConcurrentImpl(size_t senders, size_t receivers, size_t messages,
                          size_t capacity) {
    StaticThreadPool pool{5};

    Channel<int64_t> ints(capacity);

    std::atomic<int64_t> received{0};
    std::atomic<int64_t> sent{0};

    std::atomic<size_t> barrier{0};

    for (size_t i = 0; i < receivers; ++i) {
      Spawn(pool, [&]() {
        // One-pass barrier
        barrier.fetch_add(1);
        while (barrier != senders + receivers) {
          Yield();
        }

        while (true) {
          int value = ints.Receive();
          if (value == -1) {
            break;
          }
          received.fetch_add(value);
        }
      });
    }

    std::atomic<size_t> senders_done{0};

    for (size_t j = 0; j < senders; ++j) {
      Spawn(pool, [&]() {
        // One-pass barrier
        barrier.fetch_add(1);
        while (barrier != senders + receivers) {
          Yield();
        }

        for (size_t k = 1; k <= messages; ++k) {
          ints.Send(static_cast<int64_t>(k));
          sent.fetch_add(k);
        }
        if (++senders_done == senders) {
          // Last sender: send poison pills to receivers
          for (size_t i = 0; i < receivers; ++i) {
            ints.Send(-1);
          }
        }
      });
    }

    pool.Join();

    ASSERT_EQ(senders_done.load(), senders);
    ASSERT_EQ(sent.load(), received.load());
  }

  TEST(ConcurrentLargeBuffer1, kLongTestOptions) {
    TestConcurrentImpl(5, 5, 20'000, 256);
  }

  TEST(ConcurrentLargeBuffer2, kLongTestOptions) {
    TestConcurrentImpl(2, 8, 20'000, 256);
  }

  TEST(ConcurrentLargeBuffer3, kLongTestOptions) {
    TestConcurrentImpl(8, 2, 20'000, 256);
  }

  TEST(ConcurrentSmallBuffer1, kLongTestOptions) {
    TestConcurrentImpl(5, 5, 100'000, 10);
  }

  TEST(ConcurrentSmallBuffer2, kLongTestOptions) {
    TestConcurrentImpl(2, 6, 100'000, 10);
  }

  TEST(ConcurrentSmallBuffer3, kLongTestOptions) {
    TestConcurrentImpl(6, 2, 100'000, 10);
  }

  TEST(ConcurrentOneItemBuffer1, kLongTestOptions) {
    // Unbounded channel
    TestConcurrentImpl(4, 4, 50'000, 1);
  }

  TEST(ConcurrentOneItemBuffer2, kLongTestOptions) {
    TestConcurrentImpl(2, 6, 50'000, 1);
  }

  TEST(ConcurrentOneItemBuffer3, kLongTestOptions) {
    TestConcurrentImpl(6, 2, 50'000, 1);
  }

  class FifoTester {
    using ProducerId = size_t;

    struct Message {
      ProducerId producer_id;
      int value;
    };

   public:
    FifoTester(size_t threads, size_t capacity)
        : pool_{threads}, messages_{capacity} {
    }

    void RunTest(size_t producers) {
      // Single consumer
      Spawn(pool_, [this, producers]() {
        Consume(producers);
      });

      // Producers
      for (size_t id = 0; id < producers; ++id) {
        Spawn(pool_, [this, id]() {
          Produce(id);
        });
      }

      pool_.Join();

      // Print report

      std::cout << "Sends: " << sends_.load()
                << ", Receives: " << receives_.load() << std::endl;

      ASSERT_EQ(sends_.load(), receives_.load());
    }

   private:
    void Consume(size_t producers) {
      size_t left = producers;

      while (left > 0) {
        Message message = messages_.Receive();
        if (message.value == -1) {
          if (--left == 0) {
            break;
          };
        } else {
          auto last_value = GetLastValue(message.producer_id);
          ASSERT_TRUE(message.value > last_value);
          last_values_[message.producer_id] = message.value;
          receives_.fetch_add(1);
        }
      }
    }

    void Produce(ProducerId id) {
      int value = 0;
      while (wheels::test::KeepRunning()) {
        messages_.Send({id, ++value});
        sends_.fetch_add(1);
      }
      messages_.Send({id, -1});
    }

    int GetLastValue(ProducerId id) {
      auto it = last_values_.find(id);
      if (it != last_values_.end()) {
        return it->second;
      } else {
        return 0;
      }
    }

   private:
    StaticThreadPool pool_;
    Channel<Message> messages_;
    std::map<ProducerId, int> last_values_;
    std::atomic<size_t> sends_{0};
    std::atomic<size_t> receives_{0};
  };

  TEST(Fifo, wheels::test::TestOptions{}.TimeLimit(5s)) {
    FifoTester tester{/*threads=*/3, /*capacity=*/10};
    tester.RunTest(/*producers=*/5);
  }
}

TEST_SUITE(Select) {
  SIMPLE_TEST(JustWorks) {
    StaticThreadPool pool{4};

    Channel<int> ints{5};
    Channel<std::string> strs{7};

    Spawn(pool, [&]() {
      for (size_t i = 0; i < 5; ++i) {
        auto value = Select(ints, strs);
        switch (value.index()) {
          case 0:
            std::cout << "Received int " << std::get<0>(value) << std::endl;
            break;
          case 1:
            std::cout << "Received str " << std::get<1>(value) << std::endl;
            break;
        }
      }
    });

    Spawn(pool, [&]() {
      strs.Send("Hello!");
      std::this_thread::sleep_for(100ms);
      ints.Send(1);
      std::this_thread::sleep_for(100ms);
      strs.Send("Test");
      std::this_thread::sleep_for(100ms);
      ints.Send(42);
      std::this_thread::sleep_for(100ms);
      ints.Send(101);
    });

    pool.Join();
  }

  SIMPLE_FIBER_TEST(MoveOnly, 1) {
    Channel<MoveOnly<Tag1>> xs{3};
    Channel<MoveOnly<Tag2>> ys{3};

    xs.Send({"Hello"});
    ys.Send({"World"});

    auto value = Select(xs, ys);
    WHEELS_UNUSED(value);
  }

#if !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)

  SIMPLE_TEST(Blocking1) {
    StaticThreadPool pool{4};

    wheels::ProcessCPUTimer timer;

    Channel<int> xs{3};
    Channel<int> ys{3};

    Spawn(pool, [&]() {
      auto value = Select(xs, ys);
      ASSERT_EQ(value.index(), 0);
      ASSERT_EQ(std::get<0>(value), 1);
    });

    Spawn(pool, [&]() {
      std::this_thread::sleep_for(1s);
      xs.Send(1);
    });

    Spawn(pool, [&]() {
      std::this_thread::sleep_for(2s);
      ys.Send(2);
    });

    pool.Join();

    ASSERT_TRUE(timer.Elapsed() < 50ms);
  }

  SIMPLE_TEST(Blocking2) {
    StaticThreadPool pool{4};

    wheels::ProcessCPUTimer timer;

    Channel<int> xs{3};
    Channel<int> ys{3};

    Spawn(pool, [&]() {
      auto value = Select(xs, ys);
      ASSERT_EQ(value.index(), 1);
      ASSERT_EQ(std::get<1>(value), 2);
    });

    Spawn(pool, [&]() {
      std::this_thread::sleep_for(2s);
      xs.Send(1);
    });

    Spawn(pool, [&]() {
      std::this_thread::sleep_for(1s);
      ys.Send(2);
    });

    pool.Join();

    ASSERT_TRUE(timer.Elapsed() < 50ms);
  }

#endif

  class SelectTester {
    class StartLatch {
     public:
      void Release() {
        started_.store(true);
      }

      void Await() {
        while (!started_.load()) {
          Yield();
        }
      }

     private:
      std::atomic<bool> started_{false};
    };

   public:
    explicit SelectTester(size_t threads) : pool_(threads) {
    }

    void AddChannel(size_t capacity) {
      channels_.emplace_back(capacity);
    }

    void Produce(size_t i) {
      Spawn(pool_, [this, i]() {
        Producer(i);
      });
    }

    void Receive(size_t i) {
      Spawn(pool_, [this, i]() {
        ReceiveConsumer(i);
      });
    }

    void Select(size_t i, size_t j) {
      Spawn(pool_, [this, i, j]() {
        SelectConsumer(i, j);
      });
    }

    void RunTest() {
      twist::fault::GetAdversary()->Reset();

      // Release all fibers
      start_latch_.Release();

      pool_.Join();

      // Print report
      std::cout << "Sends #: " << sends_.load() << std::endl;
      std::cout << "Receives #: " << receives_.load() << std::endl;

      ASSERT_EQ(sends_.load(), receives_.load());

      std::cout << "Produced: " << total_produced_.load() << std::endl;
      std::cout << "Consumed: " << total_consumed_.load() << std::endl;

      ASSERT_EQ(total_produced_.load(), total_consumed_.load());

      twist::fault::GetAdversary()->PrintReport();
    }

   private:
    void SelectConsumer(size_t i, size_t j) {
      start_latch_.Await();

      auto xs = channels_[i];
      auto ys = channels_[j];

      bool xs_done = false;
      bool ys_done = false;

      size_t iter = 0;

      while (true) {
        if (++iter % 7 == 0) {
          Yield();
        }

        auto selected_value = mtf::fibers::Select(xs, ys);

        switch (selected_value.index()) {
          case 0: {
            int x = std::get<0>(selected_value);
            if (x == -1) {
              xs_done = true;
              xs.Send(-1);
            } else {
              total_consumed_.fetch_add(x);
              receives_.fetch_add(1);
            }
            break;
          }
          case 1: {
            int y = std::get<1>(selected_value);
            if (y == -1) {
              ys_done = true;
              ys.Send(-1);
            } else {
              total_consumed_.fetch_add(y);
              receives_.fetch_add(1);
            }
            break;
          }
        }

        if (xs_done || ys_done) {
          break;
        }
      }

      if (xs_done) {
        ReceiveConsumer(j);
      } else {
        ReceiveConsumer(i);
      }
    }

    void ReceiveConsumer(size_t i) {
      start_latch_.Await();

      auto xs = channels_[i];

      size_t iter = 0;

      while (true) {
        if (++iter % 5 == 0) {
          Yield();
        }

        auto value = xs.Receive();
        if (value == -1) {
          xs.Send(-1);
          break;
        }

        total_consumed_.fetch_add(value);
        receives_.fetch_add(1);
      }
    }

    void Producer(size_t i) {
      start_latch_.Await();

      auto xs = channels_[i];
      int value = 0;
      while (wheels::test::KeepRunning()) {
        ++value;
        xs.Send(value);
        total_produced_.fetch_add(value);
        sends_.fetch_add(1);
      }
      xs.Send(-1);  // Poison pill
    }

   private:
    StaticThreadPool pool_;

    std::vector<Channel<int>> channels_;

    StartLatch start_latch_;

    std::atomic<int64_t> sends_{0};
    std::atomic<int64_t> receives_{0};

    std::atomic<int64_t> total_produced_{0};
    std::atomic<int64_t> total_consumed_{0};
  };

  TEST(ConcurrentSelects, wheels::test::TestOptions().TimeLimit(5s)) {
    SelectTester tester{/*threads=*/4};

    tester.AddChannel(7);
    tester.AddChannel(8);

    tester.Produce(0);
    tester.Produce(1);

    tester.Select(0, 1);
    tester.Select(0, 1);

    tester.RunTest();
  }

  TEST(Deadlock, wheels::test::TestOptions().TimeLimit(5s)) {
    SelectTester tester{/*threads=*/4};

    tester.AddChannel(17);
    tester.AddChannel(17);

    tester.Produce(0);
    tester.Produce(1);

    tester.Select(0, 1);
    tester.Select(1, 0);

    tester.RunTest();
  }

  TEST(MixSelectsAndReceives, wheels::test::TestOptions().TimeLimit(5s)) {
    SelectTester tester{/*threads=*/4};

    tester.AddChannel(11);
    tester.AddChannel(9);

    tester.Produce(0);
    tester.Produce(1);

    tester.Select(0, 1);
    tester.Select(1, 0);
    tester.Receive(0);
    tester.Receive(1);

    tester.RunTest();
  }

  TEST(OverlappedSelects, wheels::test::TestOptions().TimeLimit(5s)) {
    SelectTester tester{/*threads=*/4};

    tester.AddChannel(11);
    tester.AddChannel(12);
    tester.AddChannel(13);

    tester.Produce(0);
    tester.Produce(1);
    tester.Produce(2);

    tester.Select(0, 1);
    tester.Select(1, 2);
    tester.Select(2, 0);

    tester.RunTest();
  }

  TEST(Hard, wheels::test::TestOptions().TimeLimit(5s)) {
    SelectTester tester{/*threads=*/4};

    tester.AddChannel(7);
    tester.AddChannel(9);
    tester.AddChannel(8);

    tester.Produce(0);
    tester.Produce(1);
    tester.Produce(2);

    tester.Select(0, 1);
    tester.Select(1, 0);
    tester.Select(1, 2);
    tester.Select(2, 1);
    tester.Select(2, 0);
    tester.Select(0, 2);

    tester.Receive(0);
    tester.Receive(1);
    tester.Receive(1);
    tester.Receive(2);

    tester.RunTest();
  }

  SIMPLE_FIBER_TEST(SelectFairness, 1) {
    static const size_t kIterations = 10000;

    Channel<int> xs{5};
    Channel<int> ys{5};

    xs.Send(1);
    ys.Send(2);

    int balance = 0;

    for (size_t i = 0; i < kIterations; ++i) {
      auto value = Select(xs, ys);
      switch (value.index()) {
        case 0:
          ASSERT_EQ(std::get<0>(value), 1);
          xs.Send(1);
          ++balance;
          break;
        case 1:
          ASSERT_EQ(std::get<1>(value), 2);
          ys.Send(2);
          --balance;
          break;
      }
    }

    ASSERT_TRUE(std::abs(balance) < 1000);
  }
}
