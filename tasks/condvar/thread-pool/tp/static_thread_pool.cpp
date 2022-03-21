#include <tp/static_thread_pool.hpp>

#include <tp/helpers.hpp>

#include <twist/util/thread_local.hpp>

namespace tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<StaticThreadPool> pool;

////////////////////////////////////////////////////////////////////////////////

StaticThreadPool::StaticThreadPool(size_t workers)
    : workers_(), planned_work_counter_(0) {
  while (workers-- > 0) {
    workers_.emplace_back([this] {
      Work();
    });
  }
}

StaticThreadPool::~StaticThreadPool() {
  assert(planned_work_counter_ == 0);
}

void StaticThreadPool::Submit(Task task) {
  planned_work_counter_++;
  tasks_.Put(std::move(task));
}

void StaticThreadPool::Join() {
  while (planned_work_counter_ != 0) {
    planned_work_counter_.FutexWait(0);
  }
  tasks_.Close();
  for (auto& worker : workers_) {
    worker.join();
  }
}

void StaticThreadPool::Shutdown() {
  tasks_.Cancel();
  for (auto& worker : workers_) {
    worker.join();
  }
  planned_work_counter_.store(0);
}

void StaticThreadPool::Work() {
  pool = this;

  while (true) {
    auto task = tasks_.Take();
    if (!task) {
      break;
    }
    ExecuteHere(task.value());
    if (--planned_work_counter_ == 0) {
      planned_work_counter_.notify_one();
    }
  }
}

StaticThreadPool* StaticThreadPool::Current() {
  return static_cast<bool>(pool) ? pool : nullptr;
}

}  // namespace tp
