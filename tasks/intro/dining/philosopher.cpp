#include "philosopher.hpp"

#include <twist/test/inject_fault.hpp>

namespace dining {

Philosopher::Philosopher(Table& table, size_t seat)
    : table_(table),
      seat_(seat),
      left_fork_(table_.LeftFork(seat)),
      right_fork_(table_.RightFork(seat)) {
}

void Philosopher::EatThenThink() {
  AcquireForks();
  Eat();
  ReleaseForks();
  Think();
}

// Acquire left_fork_ and right_fork_
void Philosopher::AcquireForks() {
  if (seat_ == 0) {
    left_fork_.lock();
    right_fork_.lock();
  } else {
    right_fork_.lock();
    left_fork_.lock();
  }
}

void Philosopher::Eat() {
  table_.AccessPlate(seat_);
  // Try to provoke data race
  table_.AccessPlate(table_.ToRight(seat_));
  ++meals_;
}

// Release left_fork_ and right_fork_
void Philosopher::ReleaseForks() {
  right_fork_.unlock();
  left_fork_.unlock();
}

void Philosopher::Think() {
  twist::test::InjectFault();
}

}  // namespace dining
