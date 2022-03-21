#include <mtf/thread_pool/static_thread_pool.hpp>

#include <wheels/test/test_framework.hpp>

using mtf::tp::StaticThreadPool;

TEST_SUITE(StaticThreadPool) {
  SIMPLE_TEST(HelloWorld) {
    StaticThreadPool pool{1};

    bool done = false;

    pool.Submit([&]() {
      std::this_thread::sleep_for(1s);
      std::cout << "Hello, World!" << std::endl;
      done = true;
    });

    pool.Join();

    ASSERT_TRUE(done);
  }

  SIMPLE_TEST(SubmitFromPool) {
    StaticThreadPool pool{1};

    bool done = false;

    auto task = [&]() {
      mtf::tp::Current()->Submit([&]() {
        done = true;
      });
    };

    pool.Submit(task);

    pool.Join();

    ASSERT_TRUE(done);
  }

//  SIMPLE_TEST(SubmitContinutation) {
//    StaticThreadPool pool{4};
//
//    bool done = false;
//
//    pool.Submit([&]() {
//      mtf::tp::Current()->SubmitContinuation([&]() {
//        done = true;
//      });
//      std::this_thread::sleep_for(1s);
//      ASSERT_FALSE(done);
//    });
//
//    pool.Join();
//
//    ASSERT_TRUE(done);
//  }

  SIMPLE_TEST(Join) {
    StaticThreadPool pool{3};

    std::atomic<size_t> tasks{0};

    for (size_t i = 0; i < 4; ++i) {
      pool.Submit([&]() {
        std::this_thread::sleep_for(1s);
        ++tasks;
      });
    }

    pool.Join();

    ASSERT_EQ(tasks.load(), 4);
  }
}
