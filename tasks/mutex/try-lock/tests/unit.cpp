#include "../ticket_lock.hpp"

#include <twist/test/test.hpp>

using solutions::TicketLock;

TEST_SUITE(TicketTryLock) {
  SIMPLE_TWIST_TEST(LockUnlock) {
    TicketLock ticket_lock;

    ticket_lock.Lock();
    ticket_lock.Unlock();
  }

  SIMPLE_TWIST_TEST(SequentialLockUnlock) {
    TicketLock ticket_lock;

    ticket_lock.Lock();
    ticket_lock.Unlock();
    ticket_lock.Lock();
    ticket_lock.Unlock();
  }

  SIMPLE_TWIST_TEST(TryLock) {
    TicketLock ticket_lock;

    ASSERT_TRUE(ticket_lock.TryLock());
    ASSERT_FALSE(ticket_lock.TryLock());
    ticket_lock.Unlock();
    ASSERT_TRUE(ticket_lock.TryLock());
    ticket_lock.Unlock();
    ticket_lock.Lock();
    ASSERT_FALSE(ticket_lock.TryLock());
  }
}

RUN_ALL_TESTS()
