#include <mtf/fibers/core/stacks.hpp>

#include <utility>

using context::Stack;

namespace mtf::fibers {

static Stack AllocateNewStack() {
  static const size_t kStackPages = 16;
  return Stack::AllocatePages(kStackPages);
}

Stack AllocateStack() {
  return AllocateNewStack();  // Your code goes here
}

void ReleaseStack(Stack stack) {
  Stack released{std::move(stack)};  // Your code goes here
}

}  // namespace mtf::fibers
