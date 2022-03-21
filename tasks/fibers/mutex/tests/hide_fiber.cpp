#include <type_traits>

#include <mtf/fibers/sync/mutex.hpp>
#include <mtf/fibers/sync/condvar.hpp>

struct NotFound {};

using Fiber = NotFound;

namespace mtf::fibers {

static_assert(std::is_same_v<Fiber, NotFound>, "Fiber included =(");

}  // namespace mtf::fibers
