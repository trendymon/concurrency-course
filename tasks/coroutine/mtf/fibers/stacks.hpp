#pragma once

#include <context/stack.hpp>

namespace mtf::fibers {

context::Stack AcquireStack();
void ReleaseStack(context::Stack stack);

}  // namespace mtf::fibers
