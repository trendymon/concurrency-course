#pragma once

#include "semaphore.hpp"

#include <deque>

namespace solutions {

// Bounded Blocking Multi-Producer/Multi-Consumer (MPMC) Queue

template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t capacity)
      : capacity_(capacity),
        buffer_(),
        mutex_(1),
        is_empty_(0),
        is_full_(capacity) {
  }

  // Inserts the specified element into this queue,
  // waiting if necessary for space to become available.
  void Put(T value) {
    is_full_.Acquire();

    mutex_.Acquire();
    buffer_.emplace_back(std::move(value));
    mutex_.Release();

    is_empty_.Release();
  }

  // Retrieves and removes the head of this queue,
  // waiting if necessary until an element becomes available
  T Take() {
    is_empty_.Acquire();

    mutex_.Acquire();
    T result(std::move(buffer_.front()));
    buffer_.pop_front();
    mutex_.Release();

    is_full_.Release();

    return result;
  }

 private:
  size_t const capacity_;
  std::deque<T> buffer_;
  Semaphore mutex_;
  Semaphore is_empty_;
  Semaphore is_full_;
};

}  // namespace solutions
