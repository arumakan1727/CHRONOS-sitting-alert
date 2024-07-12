#pragma once
#include <Arduino.h>

namespace armkn {

using MilliTimeUInt = decltype(millis());
static_assert(std::is_unsigned_v<MilliTimeUInt>);

enum class Result : uint8_t { Ok, Err };

using Task = void (*)();

class RepeatCount {
 public:
  using repeat_t = unsigned int;

  static const RepeatCount INF;

  static constexpr RepeatCount of(repeat_t count) { return RepeatCount(count); }

  inline constexpr bool is_infinite() const { return *this == INF; }

  inline constexpr operator repeat_t() const { return count; }

  inline constexpr bool operator==(const RepeatCount &other) const {
    return count == other.count;
  }

  inline constexpr bool operator!=(const RepeatCount &other) const {
    return !(*this == other);
  }

 private:
  RepeatCount() = default;
  explicit constexpr RepeatCount(repeat_t count) : count(count) {}

  repeat_t count;
};

inline const RepeatCount RepeatCount::INF = RepeatCount(-1);

}  // namespace armkn
