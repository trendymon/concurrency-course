#include <mtf/fibers/core/api.hpp>

#include <mtf/fibers/core/fiber.hpp>

#include <wheels/support/assert.hpp>

namespace mtf::fibers {

void Spawn(Scheduler& scheduler, Routine routine) {
  Fiber::Spawn(std::move(routine), scheduler);
}

Scheduler& GetCurrentScheduler() {
  auto* pool = tp::Current();
  WHEELS_VERIFY(pool != nullptr, "Not in scheduler");
  return *pool;
}

void Spawn(Routine routine) {
  Spawn(GetCurrentScheduler(), std::move(routine));
}

void Yield() {
  Fiber::AccessCurrent().Yield();
}

}  // namespace mtf::fibers
