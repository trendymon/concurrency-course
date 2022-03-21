#pragma once

#include <mtf/fibers/sync/channel_impl.hpp>

#include <memory>

namespace mtf::fibers {

namespace detail {

template <typename X, typename Y>
class Selector;

}  // namespace detail

// Buffered MP/MC channel
// https://tour.golang.org/concurrency/3

// Does not support void type
// Use wheels::Unit instead (from <wheels/support/unit.hpp>)

template <typename T>
class Channel {
  using Impl = detail::ChannelImpl<T>;

  template <typename X, typename Y>
  friend class detail::Selector;

 public:
  // Bounded channel, `capacity` > 0
  explicit Channel(size_t capacity) : impl_(std::make_shared<Impl>(capacity)) {
  }

  // Blocking
  void Send(T value) {
    impl_->Send(std::move(value));
  }

  // Blocking
  T Receive() {
    return impl_->Receive();
  }

 private:
  std::shared_ptr<Impl> impl_;
};

}  // namespace mtf::fibers
