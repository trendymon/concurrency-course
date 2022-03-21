#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

// std::lock_guard, std::unique_lock
#include <mutex>
#include <cstdint>

#include <iostream>

namespace solutions {

// CyclicBarrier allows a set of threads to all wait for each other
// to reach a common barrier point

// The barrier is called cyclic because
// it can be re-used after the waiting threads are released.

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t participants)
      : participants_(participants), counter_(0) {
    // Not implemented
  }

  // Blocks until all participants have invoked Arrive()
  void Arrive() {
    DerivedPhase();
    DepartedPhase();
  }

 private:
  void DerivedPhase() {
    std::unique_lock lock(mutex_);
    if (++counter_ == participants_) {
      is_open_flag = true;
      is_open_.notify_all();
    } else {
      is_open_.wait(lock, [this]{
        return is_open_flag;
      });
    }
  }

  void DepartedPhase() {
    std::unique_lock lock(mutex_);
    if (--counter_ == 0) {
      is_open_flag = false;
      is_close_.notify_all();
    } else {
      is_close_.wait(lock, [this]{
        return !is_open_flag;
      });
    }
  }

  size_t const participants_;
  size_t counter_;
  bool is_open_flag{false};
  twist::stdlike::condition_variable is_open_;
  twist::stdlike::condition_variable is_close_;
  twist::stdlike::mutex mutex_{};
};

}  // namespace solutions
