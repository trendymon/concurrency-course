#pragma once

#include <twist/stdlike/mutex.hpp>

namespace util {

//////////////////////////////////////////////////////////////////////

// Safe API for mutual exclusion

template <typename T>
class Mutexed {
  using MutexImpl = twist::stdlike::mutex;

  class UniqueRef {
    // Your code goes here
   public:
    // Non-copyable
    UniqueRef(T & guarded, MutexImpl &mutex) : guarded_(guarded), guard_(mutex)  {}
    UniqueRef(const UniqueRef&) = delete;

    // Non-movable
    UniqueRef(UniqueRef&&) = delete;

    // operator*
    auto operator*() const {
      return guarded_;
    }

    // operator->

    auto operator->() const {
      return &guarded_;
    }

   private:
    T& guarded_;
    std::lock_guard<MutexImpl> guard_;
  };

 public:
  // https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
  template <typename... Args>
  Mutexed(Args&&... args) : object_(std::forward<Args>(args)...) {
  }

  UniqueRef Lock() {
    return UniqueRef(object_, mutex_);
  }

 private:
  T object_;
  MutexImpl mutex_;  // Guards access to object_
};

//////////////////////////////////////////////////////////////////////

// Helper function for single operations over shared object:
// Usage:
//   Mutexed<vector<int>> ints;
//   Locked(ints)->push_back(42);

template <typename T>
auto Locked(Mutexed<T>& object) {
  return object.Lock();
}

}  // namespace util
