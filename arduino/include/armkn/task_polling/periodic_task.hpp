#include <Arduino.h>
#include <vector>

#include <armkn/types.hpp>

namespace armkn {

class PeriodicTask {
 public:
  const MilliTimeUInt period;
  const Task task;

 protected:
  MilliTimeUInt last_executed_at = 0;

 public:
  PeriodicTask(MilliTimeUInt period, Task task) : period(period), task(task) {}

  void tick() {
    // 32bit unsigned int
    // を仮定した場合、約49日でオーバーフローするけどヨシ👈😺⛑️👷 (現場ネコ)
    const auto now = millis();
    if (now - last_executed_at > period) {
      last_executed_at = now;
      task();
    }
  }
};

}  // namespace armkn
