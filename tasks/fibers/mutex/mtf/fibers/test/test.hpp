#pragma once

#include <wheels/test/test_framework.hpp>

#include <mtf/thread_pool/static_thread_pool.hpp>

#define SIMPLE_FIBER_TEST(name, threads)            \
  void FiberTest##name(void);                       \
  SIMPLE_TEST(name) {                               \
    ::mtf::tp::StaticThreadPool scheduler{threads}; \
    ::mtf::fibers::Spawn(scheduler, []() {          \
      FiberTest##name();                            \
    });                                             \
    scheduler.Join();                               \
  }                                                 \
  void FiberTest##name()
