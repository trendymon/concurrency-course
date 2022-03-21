//
// Created by q on 15.02.2022.
//

#include <atomic>

#ifndef CONCURRENCY_COURSE_TASSPINLOCK_H
#define CONCURRENCY_COURSE_TASSPINLOCK_H

class TASSpinLock {
 public:
  TASSpinLock();
  void lock();
  void unlock();

 private:
  std::atomic<bool> atomic_;
};

#endif  // CONCURRENCY_COURSE_TASSPINLOCK_H
