#pragma once

#include "atomic.hpp"

#include <wheels/support/cpu.hpp>

namespace solutions {

// Naive Test-and-Set (TAS) spinlock

class TASSpinLock {
 public:
  void Lock() {
    while (AtomicExchange(&locked_, 1) != 0) {
    //while (locked_.Exchange(1) != 0) {
      wheels::SpinLockPause();
    }
  }

  bool TryLock() {
    /*if (locked_.Load() == 1) {
      return false;
    }*/
    if (AtomicLoad(&locked_) == 1) {
      return false;
    }
    Lock();
    return true;
  }

  void Unlock() {
    AtomicStore(&locked_, 0);
    //locked_.Store(0);
  }

 private:
  AtomicInt64 locked_ = 0;
  //stdlike::Atomic locked_{0};
};

}  // namespace solutions
