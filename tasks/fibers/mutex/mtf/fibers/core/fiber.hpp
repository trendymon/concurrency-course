#pragma once

#include <mtf/fibers/core/api.hpp>

#include <mtf/coroutine/impl.hpp>
#include <context/stack.hpp>

namespace mtf::fibers {

class Fiber {
 public:
  Fiber(Routine routine, Scheduler& scheduler);
  ~Fiber();

  static Fiber& AccessCurrent();

  // ~ System calls

  static void Spawn(Routine routine, Scheduler& scheduler);

  void Yield();
  void Suspend();  // Better API?
  void Resume();

 private:
  // Similar to those in the TinyFibers scheduler
  void Stop();
  void Step();
  void Schedule();
  void Dispatch();
  void Destroy();
  void Await();

 private:
  // ???
};

}  // namespace mtf::fibers
