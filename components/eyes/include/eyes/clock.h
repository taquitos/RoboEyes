#ifndef EYES_CLOCK_H
#define EYES_CLOCK_H

#include <stdint.h>

namespace eyes {

// Minimal clock interface so real-time playback can be unit tested and so the eyes
// core stays platform-agnostic (host + ESP-IDF).
class IClock {
public:
  virtual ~IClock() = default;

  // Monotonic time in microseconds.
  virtual uint64_t now_us() const = 0;
};

// Default implementation backed by std::chrono::steady_clock.
class SteadyClock final : public IClock {
public:
  uint64_t now_us() const override;
};

} // namespace eyes

#endif

