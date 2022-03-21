#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdlib>

namespace stdlike {

class Mutex {
 public:
  void Lock() {
    auto cmpxchg = [](twist::stdlike::atomic<uint32_t>& atom, uint32_t expected, uint32_t desired) {
      uint32_t* ep = &expected;
      atom.compare_exchange_strong(*ep, desired);
      return *ep;
    };

    uint32_t counter;

    if ((counter = cmpxchg(flag, 0, 1)) != 0) {
      if (counter != 2) {
        counter = flag.exchange(2);
      }

      while (counter != 0) {
        flag.wait(2);
        counter = flag.exchange(2);
      }
    }
  }

  void Unlock() {
    if (flag.fetch_sub(1) != 1) {
      flag.store(0);
      flag.notify_one();
    }
  }

 private:
  twist::stdlike::atomic<uint32_t> flag{0};
};

}  // namespace stdlike