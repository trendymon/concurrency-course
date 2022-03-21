#pragma once

#include <mtf/coroutine/impl.hpp>

#include <context/stack.hpp>

namespace mtf::coroutine {

class SimpleCoroutine {
 public:
  SimpleCoroutine(Routine routine)
      : stack_(AllocateStack()), impl_(std::move(routine), stack_.View()) {
  }

  void Resume() {
    impl_.Resume();
  }

  static void Suspend() {
    impl::Coroutine::Suspend();
  }

  bool IsCompleted() const {
    return impl_.IsCompleted();
  }

 private:
  static context::Stack AllocateStack();

 private:
  context::Stack stack_;
  impl::Coroutine impl_;
};

}  // namespace mtf::coroutine
