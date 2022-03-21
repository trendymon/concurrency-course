#include "../ticket_lock.hpp"

#include <twist/strand/stdlike/thread.hpp>

#include <twist/test/test.hpp>
#include <twist/test/inject_fault.hpp>
#include <twist/test/random.hpp>

#include <twist/test/util/race.hpp>
#include <twist/test/util/plate.hpp>

#include <twist/util/spin_wait.hpp>

#include <wheels/test/test_framework.hpp>
#include <wheels/test/util.hpp>

#include <cstdlib>
#include <vector>
#include <chrono>

////////////////////////////////////////////////////////////////////////////////

using namespace std::chrono_literals;

////////////////////////////////////////////////////////////////////////////////

TEST_SUITE(TicketLock) {
  void Test(size_t lockers, size_t try_lockers) {
    twist::test::util::Plate plate;  // Guarded by ticket_lock
    solutions::TicketLock ticket_lock;

    twist::test::util::Race race{lockers + try_lockers};

    for (size_t i = 0; i < lockers; ++i) {
      race.Add([&]() {
        while (wheels::test::KeepRunning()) {
          ticket_lock.Lock();
          {
            // Critical section
            plate.Access();
          }
          ticket_lock.Unlock();
        }
      });
    }

    for (size_t j = 0; j < try_lockers; ++j) {
      race.Add([&]() {
        while (wheels::test::KeepRunning()) {
          {
            // Lock
            twist::util::SpinWait spin_wait;
            while (!ticket_lock.TryLock()) {
              spin_wait();
            }
          }
          {
            // Critical section
            plate.Access();
          }
          ticket_lock.Unlock();
        }
      });
    }

    race.Run();
  }

  TWIST_TEST_TL(Stress1, 10s) {
    Test(2, 2);
  }

  TWIST_TEST_TL(Stress2, 10s) {
    Test(5, 5);
  }

  TWIST_TEST_TL(Stress3, 10s) {
    Test(10, 10);
  }
}

////////////////////////////////////////////////////////////////////////////////

namespace forks {
  using Fork = solutions::TicketLock;

  class Table {
   public:
    Table(size_t seats)
        : seats_(seats),
          plates_(seats_),
          forks_(seats_) {
    }

    Fork& LeftFork(size_t seat) {
      return forks_[seat];
    }

    Fork& RightFork(size_t seat) {
      return forks_[ToRight(seat)];
    }

    size_t ToRight(size_t seat) const {
      return (seat + 1) % seats_;
    }

    void AccessPlate(size_t seat) {
      plates_[seat].Access();
    }

   private:
    size_t seats_;
    std::vector<twist::test::util::Plate> plates_;
    std::vector<Fork> forks_;
  };

  class Philosopher {
   public:
    Philosopher(Table& table, size_t seat)
        : table_(table),
          seat_(seat),
          left_fork_(table_.LeftFork(seat_)),
          right_fork_(table_.RightFork(seat_)) {
    }

    void EatThenThink() {
      AcquireForks();
      Eat();
      ReleaseForks();
      Think();
    }

   private:
    void AcquireForks() {
      while (true) {
        left_fork_.Lock();
        if (right_fork_.TryLock()) {
          ASSERT_FALSE(right_fork_.TryLock());
          return;
        } else {
          left_fork_.Unlock();
        }
      }
    }

    void Eat() {
      table_.AccessPlate(seat_);
      table_.AccessPlate(table_.ToRight(seat_));
    }

    void ReleaseForks() {
      if (twist::test::RandomUInteger(1) == 0) {
        ReleaseForks(left_fork_, right_fork_);
      } else {
        ReleaseForks(right_fork_, left_fork_);
      }
    };

    static void ReleaseForks(Fork& first, Fork& second) {
      first.Unlock();
      second.Unlock();
    }

    void Think() {
      twist::test::InjectFault();
    }

   private:
    Table& table_;
    size_t seat_;

    Fork& left_fork_;
    Fork& right_fork_;
  };

  void Test(size_t seats) {
    Table table{seats};

    twist::test::util::Race race{seats};

    for (size_t i = 0; i < seats; ++i) {
      race.Add([&, i]() {
        Philosopher plato(table, i);
        while (wheels::test::KeepRunning()) {
          plato.EatThenThink();
        }
      });
    }

    race.Run();
  }

}  // namespace philosophers

TEST_SUITE(Philosophers) {
  TWIST_TEST_TL(Stress1, 10s) {
    forks::Test(2);
  }

  TWIST_TEST_TL(Stress2, 10s) {
    forks::Test(5);
  }
}

////////////////////////////////////////////////////////////////////////////////

#if defined(TWIST_FIBERS)

TEST_SUITE(TicketLock2) {
  TWIST_ITERATE_TEST(NonBlocking, 10s) {
    size_t cs = 0;
    solutions::TicketLock ticket_lock;

    auto contender = [&]() {
      if (ticket_lock.TryLock()) {
        twist::strand::stdlike::this_thread::sleep_for(50ms);
        ++cs;
        ticket_lock.Unlock();
      }
    };

    twist::test::util::Race race{2};
    race.Add(contender);
    race.Add(contender);
    race.Run();

    ASSERT_TRUE(cs == 1);
  }
}

#endif


////////////////////////////////////////////////////////////////////////////////

RUN_ALL_TESTS()
