#include "../spinlock.hpp"

#include <twist/test/test.hpp>

#include <twist/test/util/race.hpp>
#include <twist/test/util/plate.hpp>

#include <twist/util/spin_wait.hpp>

#include <wheels/test/util.hpp>

#include <chrono>

////////////////////////////////////////////////////////////////////////////////

using namespace std::chrono_literals;

////////////////////////////////////////////////////////////////////////////////

TEST_SUITE(SpinLock) {
  void Test(size_t lockers, size_t try_lockers) {
    twist::test::util::Plate plate;  // Guarded by spinlock
    solutions::TASSpinLock spinlock;

    twist::test::util::Race race{lockers + try_lockers};

    std::cout << "Lockers: " << lockers
      << ", try_lockers: " << try_lockers << std::endl;

    for (size_t i = 0; i < lockers; ++i) {
      race.Add([&]() {
        while (wheels::test::KeepRunning()) {
          spinlock.Lock();
          plate.Access();
          spinlock.Unlock();
        }
      });
    }

    for (size_t j = 0; j < try_lockers; ++j) {
      race.Add([&]() {
        while (wheels::test::KeepRunning()) {
          twist::util::SpinWait spin_wait;
          while (!spinlock.TryLock()) {
            spin_wait();
          }
          plate.Access();
          spinlock.Unlock();
        }
      });
    }

    race.Run();

    std::cout << "Critical sections: " << plate.AccessCount() << std::endl;
  }

  TWIST_TEST_TL(Stress1, 5s) {
    Test(3, 0);
  }

  TWIST_TEST_TL(Stress2, 5s) {
    Test(0, 3);
  }

  TWIST_TEST_TL(Stress3, 5s) {
    Test(3, 3);
  }

  TWIST_TEST_TL(Stress4, 10s) {
    Test(5, 5);
  }

  TWIST_TEST_TL(Stress5, 10s) {
    Test(10, 10);
  }
}

RUN_ALL_TESTS()