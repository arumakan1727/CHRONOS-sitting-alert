#pragma once
#include <Arduino.h>
#include <vector>

#include <armkn/types.hpp>

namespace armkn {

class PeriodicDigitalSignal {
 public:
  struct PinOutput {
    uint8_t pin_output;
    MilliTimeUInt duration;
  };

 private:
  const char *const name;
  const uint8_t pin;
  const RepeatCount repeat;
  const std::vector<PinOutput> sequence;

  size_t current_index;
  MilliTimeUInt last_changed_at;
  RepeatCount::repeat_t current_repeat_count;
  bool is_enabled;

 public:
  PeriodicDigitalSignal(
    const char *name,
    uint8_t pin,
    RepeatCount repeat,
    std::initializer_list<PinOutput> sequence
  )
      : name(name),
        pin(pin),
        repeat(repeat),
        sequence(sequence),
        current_index(0),
        last_changed_at(millis()),
        current_repeat_count(0),
        is_enabled(false) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  void tick() {
    if (!is_enabled)
      return;

    const auto now = millis();
    if (now - last_changed_at < sequence[current_index].duration)
      return;

    Serial.print(name);
    Serial.println(": change digital signal to next state");

    ++current_index;

    if (current_index >= sequence.size()) {
      current_index = 0;
      ++current_repeat_count;

      if (current_repeat_count >= repeat) {
        if (repeat.is_infinite()) {
          current_repeat_count = 0;
        } else {
          stop();
          return;
        }
      }
    }
    last_changed_at = now;
    digitalWrite(pin, sequence[current_index].pin_output);
  }

  void start() {
    Serial.print(name);
    Serial.println(": start digital signal");
    is_enabled = true;
    current_index = 0;
    current_repeat_count = 0;
    last_changed_at = millis();
    digitalWrite(pin, sequence[current_index].pin_output);
  }

  void stop() {
    Serial.print(name);
    Serial.println(": stop digital signal");
    is_enabled = false;
    digitalWrite(pin, LOW);
  }

  void enable() {
    if (is_enabled)
      return;

    start();
  }

  void disable() {
    if (!is_enabled)
      return;

    stop();
  }
};

}  // namespace armkn
