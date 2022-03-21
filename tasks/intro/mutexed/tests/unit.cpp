#include "mutexed.hpp"

#include <twist/test/test.hpp>

#include <twist/stdlike/thread.hpp>

#include <twist/strand/stdlike.hpp>

#include <chrono>
#include <set>
#include <string>
#include <vector>

using namespace std::chrono_literals;

using util::Mutexed;
using util::Locked;

TEST_SUITE(Guarded) {
  SIMPLE_TWIST_TEST(Vector) {
    Mutexed<std::vector<int>> ints;

    {
      auto ref = ints.Lock();
      ASSERT_TRUE(ref->empty());
    }

    {
      auto ref = ints.Lock();

      ref->push_back(42);
      ASSERT_EQ(ref->front(), 42);
      ASSERT_EQ(ref->at(0), 42);
      ASSERT_EQ(ref->size(), 1);
    }

    {
      auto ref = ints.Lock();
      ref->push_back(99);
      ASSERT_EQ(ref->size(), 2);
    }
  }

  SIMPLE_TWIST_TEST(Set) {
    Mutexed<std::set<std::string>> strings;

    {
      auto ref = strings.Lock();
      ref->insert("Hello");
      ref->insert("World");
      ref->insert("!");
    }

    ASSERT_EQ(Locked(strings)->size(), 3);
  }

  SIMPLE_TWIST_TEST(Ctor) {
    Mutexed<std::string> str(5, '!');
    ASSERT_EQ(Locked(str)->length(), 5);
  }

  class Counter {
   public:
    void Increment() {
      size_t value = value_;
      twist::strand::stdlike::this_thread::sleep_for(1s);
      value_ = value + 1;
    }

    size_t Value() const {
      return value_;
    }

   private:
    size_t value_{0};
  };

  SIMPLE_TWIST_TEST(Counter) {
    Mutexed<Counter> counter;

    twist::stdlike::thread t1([&]() {
      Locked(counter)->Increment();
    });
    twist::stdlike::thread t2([&]() {
      Locked(counter)->Increment();
    });

    t1.join();
    t2.join();

    ASSERT_EQ(Locked(counter)->Value(), 2);
  }
}

RUN_ALL_TESTS()
