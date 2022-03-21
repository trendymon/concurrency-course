#pragma once

#include <mtf/fibers/core/suspend.hpp>

#include <wheels/support/intrusive_list.hpp>
#include <wheels/support/assert.hpp>

#include <deque>

/*
 * https://www.youtube.com/watch?v=cdpQMDgQP8Y
 * https://github.com/golang/go/issues/8899
 */

namespace mtf::fibers {

namespace detail {

template <typename T>
class ChannelImpl {
 public:
  ChannelImpl(size_t capacity) : capacity_(capacity) {
    WHEELS_VERIFY(capacity > 0, "Capacity == 0");
  }

  ~ChannelImpl() {
    // Not implemented
    // Check that receivers/senders queues are empty
  }

  void Send(T /*value*/) {
    // Not implemented
  }

  T Receive() {
    std::abort();  // Not implemented
  }

 private:
  const size_t capacity_;

  // ???
};

}  // namespace detail

}  // namespace mtf::fibers
