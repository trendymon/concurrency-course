#include <mtf/fibers/stacks.hpp>
#include "mtf/support/TASSpinLock.h"
#include <mutex>
#include <vector>
#include <utility>

using context::Stack;

namespace mtf::fibers {

static Stack AllocateNewStack() {
  static const size_t kStackPages = 8;
  return Stack::AllocatePages(kStackPages);
}

class StackPool {
 public:
  Stack Get() {
    {
      std::lock_guard guard(lock_);
      if (!pool_.empty()) {
        Stack stack(std::move(pool_.back()));
        pool_.pop_back();
        return stack;
      }
    }

    return AllocateNewStack();
  }

  void Release(Stack stack) {
    std::lock_guard guard(lock_);
    pool_.push_back(std::move(stack));
  }

 private:
  std::vector<Stack> pool_;
  TASSpinLock lock_;
};

static thread_local StackPool pool = StackPool();

Stack AcquireStack() {
  return pool.Get();  // Your code goes here
}

void ReleaseStack(Stack stack) {
  //Stack released{std::move(stack)};  // Your code goes here
  pool.Release(std::move(stack));
}

}  // namespace mtf::fibers
