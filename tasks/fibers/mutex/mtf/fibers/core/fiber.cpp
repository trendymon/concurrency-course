#include <mtf/fibers/core/fiber.hpp>

#include <mtf/fibers/core/stacks.hpp>
#include <mtf/fibers/core/handle.hpp>

#include <wheels/support/assert.hpp>
#include <wheels/support/exception.hpp>

namespace mtf::fibers {

Fiber& Fiber::AccessCurrent() {
  std::abort();  // Not implemented
}

Fiber::Fiber(Routine /*routine*/, Scheduler& /*scheduler*/) {
}

Fiber::~Fiber() {
}

void Fiber::Spawn(Routine /*routine*/, Scheduler& /*scheduler*/) {
  // Not implemented
}

// System calls

void Fiber::Yield() {
  Stop();
}

void Fiber::Suspend() {
  // Not implemented
}

void Fiber::Resume() {
  // Not implemented
}

// Scheduler ops

void Fiber::Schedule() {
  // Not implemented
}

void Fiber::Dispatch() {
  // Not implemented
}

void Fiber::Await() {
  // Not implemented
}

void Fiber::Destroy() {
  // Not implemented
}

void Fiber::Step() {
  // Not implemented
}

void Fiber::Stop() {
  // Not implemented
}

}  // namespace mtf::fibers
