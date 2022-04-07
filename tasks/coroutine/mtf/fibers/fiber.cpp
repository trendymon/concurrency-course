#include <mtf/fibers/api.hpp>

#include <mtf/coroutine/impl.hpp>
#include <mtf/fibers/stacks.hpp>

namespace mtf::fibers {

using coroutine::impl::Coroutine;

class Fiber;

static thread_local Fiber* current_fiber{nullptr};

////////////////////////////////////////////////////////////////////////////////

class Fiber {
 public:
  Fiber(Routine routine, Scheduler& scheduler)
      : fiber_stack_(AcquireStack()),
        coroutine_(std::move(routine), fiber_stack_.View()),
        scheduler_(scheduler) {
  }

  void Schedule() {
    scheduler_.Submit([this] {
      Dispatch();
    });
  }

  void Dispatch() {
    auto *prev_fiber = std::exchange(current_fiber, this);
    coroutine_.Resume();
    std::exchange(current_fiber, prev_fiber);
    if (coroutine_.IsCompleted()) {
      Destroy();
    }
  }

  void Yield() {
    scheduler_.SubmitContinuation([this] {
      Dispatch();
    });
    coroutine_.Suspend();
  }

  void Destroy() {
    ReleaseStack(std::move(fiber_stack_));
    delete this;
  }

 private:
  context::Stack fiber_stack_;
  Coroutine coroutine_;
  Scheduler& scheduler_;
};

////////////////////////////////////////////////////////////////////////////////

void Spawn(Routine routine, Scheduler& scheduler) {
  auto* fiber = new Fiber(std::move(routine), scheduler);

  fiber->Schedule();
}

void Spawn(Routine routine) {
  Spawn(std::move(routine), *Scheduler::Current());
}

void Yield() {
  current_fiber->Yield();
}

}  // namespace mtf::fibers
