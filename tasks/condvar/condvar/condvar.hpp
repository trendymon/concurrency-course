#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdint>

namespace stdlike {

class CondVar {
 public:
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex& mutex) {
    uint32_t old_signal_count = futex_;
    mutex.unlock();
    futex_.FutexWait(old_signal_count);
    mutex.lock();
  }

  void NotifyOne() {
    futex_.fetch_add(1);
    futex_.FutexWakeOne();
  }

  void NotifyAll() {
    futex_.fetch_add(1);
    futex_.FutexWakeAll();
  }

  twist::stdlike::atomic<uint32_t> futex_{0};
};

}  // namespace stdlike
