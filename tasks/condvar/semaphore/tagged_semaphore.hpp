#pragma once

#include "semaphore.hpp"

#include <cassert>

namespace solutions {

template <typename Tag>
class TaggedSemaphore {
 private:
  // No affine types in C++ =(
  class Token {
    friend class TaggedSemaphore;

   public:
    ~Token() {
      assert(!valid_);
    }

    // Non-copyable
    Token(const Token&) = delete;
    Token& operator=(const Token&) = delete;

    // Movable

    Token(Token&& that) {
      that.Invalidate();
    }

    Token& operator=(Token&& that) = delete;

   private:
    Token() = default;

    void Invalidate() {
      assert(valid_);
      valid_ = false;
    }

   private:
    bool valid_{true};
  };

  class Guard {
   public:
    explicit Guard(TaggedSemaphore& host)
      : host_(host), token_(host_.Acquire()) {
    }

    ~Guard() {
      host_.Release(std::move(token_));
    }

   private:
    TaggedSemaphore& host_;
    Token token_;
  };

 public:
  explicit TaggedSemaphore(size_t tokens)
      : impl_(tokens) {
  }

  Token Acquire() {
    impl_.Acquire();
    return Token{};
  }

  void Release(Token&& token) {
    impl_.Release();
    token.Invalidate();
  }

  Guard MakeGuard() {
    return Guard{*this};
  }

 private:
  Semaphore impl_;
};

}  // namespace solutions
