#include <mtf/fibers/sync/condvar.hpp>

namespace mtf::fibers {

void CondVar::Wait(Lock& /*lock*/) {
  // Not implemented
}

void CondVar::NotifyOne() {
  // Not implemented
}

void CondVar::NotifyAll() {
  // Not implemented
}

}  // namespace mtf::fibers
