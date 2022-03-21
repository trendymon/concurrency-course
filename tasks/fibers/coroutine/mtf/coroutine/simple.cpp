#include <mtf/coroutine/simple.hpp>

namespace mtf::coroutine {

using context::Stack;

Stack SimpleCoroutine::AllocateStack() {
  static const size_t kStackPages = 8;
  return Stack::AllocatePages(kStackPages);
}

}  // namespace mtf::coroutine
