#include <mtf/fibers/api.hpp>

#include <mtf/coroutine/impl.hpp>
#include <mtf/fibers/stacks.hpp>

namespace mtf::fibers {

using coroutine::impl::Coroutine;

////////////////////////////////////////////////////////////////////////////////

class Fiber {
 public:
  Fiber(Routine routine, Scheduler& scheduler)
      : fiber_stack_(AcquireStack()),
        coroutine_(std::move(routine), fiber_stack_.View()),
        scheduler_(scheduler) {
  }

 private:
  // Similar to those in the TinyFibers scheduler

  void Schedule() {
    // Not implemented
    scheduler_.Submit([this] {
      coroutine_.Resume();
    });
  }

  void Dispatch() {
    // Not implemented
  }

  void Destroy() {
    // Not implemented
  }

 private:
  context::Stack fiber_stack_;
  Coroutine coroutine_;
  Scheduler& scheduler_;
};

////////////////////////////////////////////////////////////////////////////////

void Spawn(Routine /*routine*/, Scheduler& /*scheduler*/) {
  // Not implemented
}

void Spawn(Routine /*routine*/) {
  // Not implemented
}

void Yield() {
  // Not implemented
}

}  // namespace mtf::fibers
