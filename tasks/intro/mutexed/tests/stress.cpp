#include "mutexed.hpp"

#include <twist/test/test.hpp>

#include <twist/test/util/race.hpp>
#include <twist/test/util/plate.hpp>

#include <wheels/test/util.hpp>

TEST_SUITE(Guarded) {
  TWIST_TEST_TL(Stress, 3s) {
    static const size_t kThreads = 3;

    // Set of hungry threads
    twist::test::util::Race race;
    // Plate shared between threads
    util::Mutexed<twist::test::util::Plate> plate;

    for (size_t i = 0; i < kThreads; ++i) {
      race.Add([&]() {
        while (wheels::test::KeepRunning()) {
          Locked(plate)->Access();
        }
      });
    }

    race.Run();
  }
}

RUN_ALL_TESTS()
