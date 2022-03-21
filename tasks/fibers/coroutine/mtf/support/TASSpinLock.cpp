//
// Created by q on 15.02.2022.
//

#include "TASSpinLock.h"

TASSpinLock::TASSpinLock(): atomic_(false) {
}

void TASSpinLock::lock() {
  while (atomic_.exchange(true)) {
    while (atomic_.load()) {
      //backoff
    }
  }
}

void TASSpinLock::unlock() {
  atomic_.store(false);
}