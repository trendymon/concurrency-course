#pragma once

#include <cstdint>
#include <string>
#include <sstream>

namespace tests::support {

class GrowingBuffer {
 public:
  size_t Size() const {
    return size_;
  }

  void Append(const char* data, size_t bytes) {
    buffer_.write(data, bytes);
    size_ += bytes;
  }

  std::string ToString() const {
    return buffer_.str();
  }

 private:
  std::ostringstream buffer_;
  size_t size_ = 0;
};

}  // namespace tests::support
