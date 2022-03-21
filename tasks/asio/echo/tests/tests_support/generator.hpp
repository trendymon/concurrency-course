#pragma once

#include <cstdint>
#include <random>
#include <string>

namespace tests::support {

class DataGenerator {
 public:
  DataGenerator(size_t bytes) : bytes_left_(bytes) {
  }

  bool HasMore() const {
    return bytes_left_ > 0;
  }

  size_t NextChunk(char* buffer, size_t limit) {
    size_t bytes = std::min(bytes_left_, limit);
    GenerateTo(buffer, bytes);
    bytes_left_ -= bytes;
    return bytes;
  }

 private:
  void GenerateTo(char* buffer, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) {
      buffer[i] = 'A' + twister_() % 26;
    }
  }

 private:
  size_t bytes_left_;
  std::mt19937 twister_{42};
};

}  // namespace tests::support