#include <mtf/fibers/core/handle.hpp>

#include <mtf/fibers/core/fiber.hpp>

#include <wheels/support/assert.hpp>

namespace mtf::fibers {

void FiberHandle::Resume() {
  WHEELS_VERIFY(IsValid(), "Invalid fiber handle");
  // Not implemented
}

}  // namespace mtf::fibers
