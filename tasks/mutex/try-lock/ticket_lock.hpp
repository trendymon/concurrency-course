#pragma once

#include <twist/stdlike/atomic.hpp>
#include <twist/util/spin_wait.hpp>

#include <cstdlib>

namespace solutions {

class TicketLock {
  using Ticket = size_t;

 public:
  // Don't change this method
  void Lock() {
    const Ticket this_thread_ticket = next_free_ticket_.fetch_add(1);

    twist::util::SpinWait spin_wait;
    while (this_thread_ticket != owner_ticket_.load()) {
      spin_wait();
    }
  }

  bool TryLock() {
    bool const isFreeLock = next_free_ticket_.load() == owner_ticket_.load();
    if (not isFreeLock) {
      return false;
    }
    Lock();
    return true;
  }

  // Don't change this method
  void Unlock() {
    // Do we actually need atomic increment here?
    owner_ticket_.fetch_add(1);
  }

 private:
  twist::stdlike::atomic<Ticket> next_free_ticket_{0};
  twist::stdlike::atomic<Ticket> owner_ticket_{0};
};

}  // namespace solutions
