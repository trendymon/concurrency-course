#include <mtf/fibers/test/test.hpp>

#include <mtf/fibers/core/api.hpp>
#include <mtf/fibers/sync/mutex.hpp>
#include <mtf/fibers/sync/condvar.hpp>

#include <twist/test/test.hpp>
#include <twist/test/runs.hpp>
#include <twist/test/random.hpp>

#include <wheels/test/util.hpp>

#include <deque>

using namespace mtf::fibers;
using mtf::tp::StaticThreadPool;

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue(size_t capacity) : capacity_(capacity) {
  }

  void Put(T value) {
    std::unique_lock lock(mutex_);

    while (buffer_.size() == capacity_) {
      not_full_.Wait(lock);
    }

    buffer_.push_back(std::move(value));

    if (twist::test::Random2()) {
      lock.unlock();
    }
    not_empty_.NotifyOne();
  }

  T Take() {
    std::unique_lock lock(mutex_);
    while (buffer_.empty()) {
      not_empty_.Wait(lock);
    }
    not_full_.NotifyOne();
    return TakeLocked();
  }

 private:
  T TakeLocked() {
    T front = std::move(buffer_.front());
    buffer_.pop_front();
    return front;
  }

 private:
  std::deque<int> buffer_;
  size_t capacity_;
  Mutex mutex_;
  CondVar not_empty_;
  CondVar not_full_;
};

void QueueStressTest(size_t threads, size_t producers, size_t consumers) {
  std::cout << "Threads = " << threads << ", producers = " << producers
            << ", consumers = " << consumers << std::endl;

  BlockingQueue<int> queue_{42};

  std::atomic<int> consumed{0};
  std::atomic<int> produced{0};
  std::atomic<int> puts{0};

  std::atomic<size_t> producers_left{producers};

  StaticThreadPool pool{threads};

  // Producers

  for (size_t i = 0; i < producers; ++i) {
    Spawn(pool, [&, i]() {
      int value = i;
      while (wheels::test::KeepRunning()) {
        queue_.Put(value);
        ++puts;
        produced.fetch_add(value);
        value += producers;
      }

      if (producers_left.fetch_sub(1) == 1) {
        // Last producer
        for (size_t j = 0; j < consumers; ++j) {
          // Put poison pill
          queue_.Put(-1);
        }
      }
    });
  }

  // Consumers

  for (size_t j = 0; j < consumers; ++j) {
    Spawn(pool, [&]() {
      while (true) {
        int value = queue_.Take();
        if (value == -1) {
          break;  // Poison pill
        }
        consumed.fetch_add(value);
      }
    });
  }

  pool.Join();

  ASSERT_EQ(consumed.load(), produced.load());

  std::cout << "Put-s completed: " << puts.load() << std::endl;
}

TWIST_TEST_RUNS(Queue, QueueStressTest)
    ->TimeLimit(5s)
    ->Run(/*threads=*/3, /*producers=*/5, /*consumers=*/1)
    ->Run(3, 1, 5)
    ->Run(5, 5, 5)
    ->Run(5, 10, 10)
    ->Run(5, 50, 10)
    ->Run(5, 10, 50)
    ->Run(5, 100, 100);
