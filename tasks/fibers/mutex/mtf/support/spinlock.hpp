#pragma once

#include <twist/stdlike/atomic.hpp>
#include <twist/util/spin_wait.hpp>

namespace mtf::support {

class SpinLock {
 public:
  void Lock() {
    // Not implemented
  }

  void Unlock() {
    // Not implemented
  }

  // NOLINTNEXTLINE
  void lock() {
    Lock();
  }

  // NOLINTNEXTLINE
  void unlock() {
    Unlock();
  }

 private:
  // ???
};

}  // namespace mtf::support
