#include <mtf/fibers/core/api.hpp>

#include <wheels/test/test_framework.hpp>

using mtf::fibers::Spawn;
using mtf::fibers::Yield;

TEST_SUITE(Stacks) {
#if !__has_feature(thread_sanitizer)

  void Recurse(size_t left) {
    mtf::fibers::Yield();

    if (left > 0) {
      Recurse(left - 1);
    }
  }

  SIMPLE_TEST(Recurse) {
    mtf::tp::StaticThreadPool scheduler{4};

    for (size_t i = 0; i < 128; ++i) {
      Spawn(scheduler, []() {
        Recurse(128);
      });
    }

    scheduler.Join();
  }

#endif

#if !__has_feature(thread_sanitizer) && !__has_feature(address_sanitizer)

  TEST(Pool, wheels::test::TestOptions().TimeLimit(10s)) {
    mtf::tp::StaticThreadPool scheduler{1};

    static const size_t kFibers = 1'000'000;

    std::atomic<size_t> counter{0};

    auto acceptor = [&]() {
      for (size_t i = 0; i < kFibers; ++i) {
        // ~ Accept client
        Yield();

        // ~ Handle request
        Spawn([&]() {
          ++counter;
        });
      }
    };

    Spawn(scheduler, acceptor);

    scheduler.Join();

    ASSERT_EQ(counter, kFibers)
  }

#endif
}
