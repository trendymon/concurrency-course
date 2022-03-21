#pragma once

#include <twist/stdlike/condition_variable.hpp>
#include <twist/stdlike/mutex.hpp>
#include <tp/blocking_queue.hpp>
#include <tp/task.hpp>

namespace tp {

void ExecuteHere(Task& task);

class CountingTasks {
 public:
  void Increase() {
    std::lock_guard<twist::stdlike::mutex> locker(mutex_);
    ++tasks_counter_;
  }

  void DecreaseAndCheck() {
    std::lock_guard<twist::stdlike::mutex> locker(mutex_);
    --tasks_counter_;
    if (tasks_counter_ == 0) {
      no_tasks_.notify_all();
    }
  }

  void WaitForNoTasks() {
    std::unique_lock<twist::stdlike::mutex> locker(mutex_);
    while (tasks_counter_ != 0) {
      no_tasks_.wait(locker);
    }
  }

 private:
  int tasks_counter_{0};
  twist::stdlike::condition_variable no_tasks_;
  twist::stdlike::mutex mutex_;
};


}  // namespace tp