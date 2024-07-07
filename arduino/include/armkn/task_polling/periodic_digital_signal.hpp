#pragma once
#include <Arduino.h>
#include <vector>

#include <armkn/types.hpp>

namespace armkn {

class PeriodicDigitalSignal {
 public:
  struct SignalState {
    int pin_output;
    MilliTimeUInt duration;
  };

 private:
  const uint8_t pin;
  const RepeatCount repeat;
  const std::vector<SignalState> sequence;

  size_t current_index;
  MilliTimeUInt last_changed_at;
  RepeatCount::repeat_t current_repeat_count;
  bool is_enabled;

 public:
  PeriodicDigitalSignal(
    uint8_t pin,
    RepeatCount repeat,
    std::initializer_list<SignalState> sequence
  )
      : pin(pin),
        repeat(repeat),
        sequence(sequence),
        current_index(0),
        last_changed_at(millis()),
        current_repeat_count(0),
        is_enabled(true) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  void tick() {
    if (!is_enabled)
      return;

    const auto now = millis();
    if (now - last_changed_at < sequence[current_index].duration)
      return;

    last_changed_at = now;
    ++current_index;

    if (current_index >= sequence.size()) {
      current_index = 0;
      ++current_repeat_count;

      if (current_repeat_count >= repeat) {
        if (repeat.is_infinite()) {
          current_repeat_count = 0;
        } else {
          disable();
          return;
        }
      }
    }

    digitalWrite(pin, sequence[current_index].pin_output);
  }

  void enable() { is_enabled = true; }

  void disable() {
    is_enabled = false;
    digitalWrite(pin, LOW);
  }

  void reset() {
    current_index = 0;
    current_repeat_count = 0;
    last_changed_at = millis();
  }
};

}  // namespace armkn
