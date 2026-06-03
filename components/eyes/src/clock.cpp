#include "eyes/clock.h"

#include <chrono>

namespace eyes {

uint64_t SteadyClock::now_us() const {
  using namespace std::chrono;
  auto now = steady_clock::now().time_since_epoch();
  return static_cast<uint64_t>(duration_cast<microseconds>(now).count());
}

} // namespace eyes

