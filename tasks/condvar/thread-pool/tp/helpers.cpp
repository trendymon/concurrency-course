#include <tp/helpers.hpp>

namespace tp {

void ExecuteHere(Task& task) {
  try {
    task();
  } catch (...) {
    // Ignore exceptions
  }
}

}  // namespace tp
