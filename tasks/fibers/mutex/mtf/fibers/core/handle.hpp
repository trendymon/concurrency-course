#pragma once

namespace mtf::fibers {

// Lightweight non-owning handle to the fiber object

class FiberHandle {
 public:
  explicit FiberHandle(void* fiber) : fiber_(fiber) {
  }

  FiberHandle() : FiberHandle(nullptr) {
  }

  static FiberHandle Invalid() {
    return FiberHandle(nullptr);
  }

  bool IsValid() const {
    return fiber_ != nullptr;
  }

  void Resume();

 private:
  void* fiber_;
};

}  // namespace mtf::fibers
