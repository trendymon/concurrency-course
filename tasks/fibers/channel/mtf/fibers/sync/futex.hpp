#pragma once

#include <mtf/fibers/core/handle.hpp>
#include <mtf/fibers/core/suspend.hpp>

#include <twist/stdlike/atomic.hpp>

#include <wheels/support/intrusive_list.hpp>

namespace mtf::fibers {

// https://eli.thegreenplace.net/2018/basics-of-futexes/

template <typename T>
class FutexLike : public twist::stdlike::atomic<T> {
 public:
  explicit FutexLike(T initial_value)
      : twist::stdlike::atomic<T>(initial_value) {
  }

  ~FutexLike() {
    // Not implemented
    // Check that wait queue is empty
  }

  // Park current fiber if value of atomic is equal to `old`
  void ParkIfEqual(T /*old*/) {
    // Not implemented
  }

  void WakeOne() {
    // Not implemented
  }

  void WakeAll() {
    // Not implemented
  }

 private:
  // ???
};

}  // namespace mtf::fibers
