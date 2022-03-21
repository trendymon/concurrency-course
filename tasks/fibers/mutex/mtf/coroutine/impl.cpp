#include <mtf/coroutine/impl.hpp>

namespace mtf::coroutine::impl {

Coroutine::Coroutine(Routine /*routine*/, context::StackView /*stack*/) {
  // Not implemented
}

void Coroutine::Resume() {
  // Not implemented
}

void Coroutine::Suspend() {
  // Not implemented
}

bool Coroutine::IsCompleted() const {
  return false;  // Not implemented
}

void Coroutine::Trampoline() {
  // Not implemented
  std::abort();
}

}  // namespace mtf::coroutine::impl
