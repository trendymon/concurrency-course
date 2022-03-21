#pragma once

#include <mtf/coroutine/routine.hpp>

#include <context/context.hpp>

#include <exception>

namespace mtf::coroutine::impl {

// Stackful asymmetric coroutine

class Coroutine {
 public:
  Coroutine(Routine routine, context::StackView stack);

  // Non-copyable
  Coroutine(const Coroutine&) = delete;
  Coroutine& operator=(const Coroutine&) = delete;

  void Resume();

  // Suspends current coroutine
  static void Suspend();

  bool IsCompleted() const;

 private:
  [[noreturn]] static void Trampoline();

 private:
  // ???
};

}  // namespace mtf::coroutine::impl
