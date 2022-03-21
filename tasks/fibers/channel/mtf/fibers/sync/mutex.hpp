#pragma once

#include <mtf/fibers/sync/futex.hpp>

namespace mtf::fibers {

class Mutex {
 public:
  void Lock() {
    // Not implemented
  }

  void Unlock() {
    // Not implemented
  }

  // BasicLockable concept

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  // ???
};

}  // namespace mtf::fibers
