#pragma once

#include "table.hpp"

namespace dining {

class Philosopher {
 public:
  Philosopher(Table& table, size_t seat);

  void EatThenThink();

  size_t Meals() const {
    return meals_;
  }

 private:
  void AcquireForks();
  void Eat();
  void ReleaseForks();
  void Think();

 private:
  Table& table_;
  size_t seat_;

  Fork& left_fork_;
  Fork& right_fork_;

  size_t meals_ = 0;
};

}  // namespace dining
