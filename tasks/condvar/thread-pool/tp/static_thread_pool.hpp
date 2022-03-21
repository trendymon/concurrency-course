#pragma once

#include <tp/blocking_queue.hpp>
#include <tp/task.hpp>

#include <twist/stdlike/thread.hpp>

namespace tp {

// Fixed-size pool of worker threads

class StaticThreadPool {
 public:
  explicit StaticThreadPool(size_t workers);
  ~StaticThreadPool();

  // Non-copyable
  StaticThreadPool(const StaticThreadPool&) = delete;
  StaticThreadPool& operator=(const StaticThreadPool&) = delete;

  // Schedules task for execution in one of the worker threads
  void Submit(Task task);

  // Graceful shutdown
  // Waits until outstanding work count has reached 0
  // and joins worker threads
  void Join();

  // Hard shutdown
  // Joins worker threads ASAP
  void Shutdown();

  // Locate current thread pool from worker thread
  static StaticThreadPool* Current();

 private:
  void Work();

  std::vector<twist::stdlike::thread> workers_;
  tp::UnboundedBlockingQueue<Task> tasks_;
  twist::thread::FutexedAtomic<uint32_t> planned_work_counter_;
};

inline StaticThreadPool* Current() {
  return StaticThreadPool::Current();
}

}  // namespace tp
