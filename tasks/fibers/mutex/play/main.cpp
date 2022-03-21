#include <mtf/fibers/core/api.hpp>
#include <mtf/fibers/sync/mutex.hpp>

#include <iostream>

using mtf::fibers::Spawn;
using mtf::fibers::Mutex;

int main() {
  mtf::tp::StaticThreadPool scheduler{/*threads=*/4};
  Mutex mutex;

  Spawn(scheduler, []() {
    std::cout << "Hello, world!" << std::endl;
  });

  scheduler.Join();

  return 0;
}
