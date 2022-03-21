#pragma once

#include <mtf/fibers/sync/mutex.hpp>
#include <mtf/fibers/sync/futex.hpp>

// std::unique_lock
#include <mutex>

namespace mtf::fibers {

class CondVar {
  using Lock = std::unique_lock<Mutex>;

 public:
  void Wait(Lock& lock);

  void NotifyOne();
  void NotifyAll();

 private:
  // ???
};

}  // namespace mtf::fibers
