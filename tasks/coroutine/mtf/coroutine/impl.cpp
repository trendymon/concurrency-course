#include <mtf/coroutine/impl.hpp>
namespace mtf::coroutine::impl {

static thread_local Coroutine* current = nullptr;

Coroutine::Coroutine(Routine routine, context::StackView stack)
    : payload_(std::move(routine)),
      stack_view_(stack),
      coroutine_context_(),
      caller_context_(),
      is_completed_(false), exception_() {
  coroutine_context_.Setup(stack_view_, Coroutine::Trampoline);
}

void Coroutine::Resume() {
  auto* last = std::exchange(current, this);
  caller_context_.SwitchTo(coroutine_context_);
  std::exchange(current, last);

  if (exception_) {
    std::rethrow_exception(exception_);
  }
}

void Coroutine::Suspend() {
  current->coroutine_context_.SwitchTo(current->caller_context_);
}

bool Coroutine::IsCompleted() const {
  return is_completed_;
}

void Coroutine::Trampoline() {
  current->coroutine_context_.AfterStart();
  try {
    current->payload_();
  } catch (...) {
    current->exception_ = std::current_exception();
  }
  current->is_completed_ = true;
  current->coroutine_context_.ExitTo(current->caller_context_);
  std::abort();
}

}  // namespace mtf::coroutine::impl
