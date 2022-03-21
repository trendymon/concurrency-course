#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <optional>
#include <queue>

namespace tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedBlockingQueue {
 public:
  bool Put(T value) {
    std::lock_guard lock(mutex_);

    if (!is_closed_) {
      queue_.push(std::move(value));
      is_empty_.notify_one();
      return true;
    }

    return false;
  }

  std::optional<T> Take() {
    std::unique_lock lock(mutex_);

    while (!is_closed_ && queue_.empty()) {
      is_empty_.wait(lock);
    }

    if (is_closed_ && queue_.empty()) {
      return std::nullopt;
    }

    T result = std::move(queue_.front());
    queue_.pop();
    return result;
  }

  void Close() {
    std::lock_guard lock(mutex_);
    CloseImpl(/*clear=*/false);
  }

  void Cancel() {
    std::lock_guard lock(mutex_);
    CloseImpl(/*clear=*/true);
  }

 private:
  void CloseImpl(bool clear) {
    is_closed_ = true;

    if (clear) {
      queue_ = std::queue<T>();
    }
    is_empty_.notify_all();
  }

 private:
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable is_empty_;
  bool is_closed_{false};
  std::queue<T> queue_;
};

}  // namespace tp
