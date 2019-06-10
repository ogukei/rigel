
#ifndef RIGEL_GRAPHICS_RENDER_TIMER_H_
#define RIGEL_GRAPHICS_RENDER_TIMER_H_

#include <functional>

namespace rigel {

class IntervalTimerState;

class IntervalTimer {
 private:
  IntervalTimerState *state_;
 public:
  IntervalTimer(double delay_sec, std::function<void(double)> f);
  ~IntervalTimer();
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_TIMER_H_
