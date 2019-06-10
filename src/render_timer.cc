
#include "render_timer.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <chrono>
#include <iostream>

extern "C" {
#include <time.h>
#include <pthread.h>
void *RGLIntervalTimerThreadEntry(void *state);
}

static inline void RGLSleep(time_t nsec) {
    struct timespec delay = { nsec / 1000000000, nsec % 1000000000 };
    pselect(0, NULL, NULL, NULL, &delay, NULL);
}

namespace rigel {
class IntervalTimerState {
  pthread_t thread_;
  // internal state
  volatile bool running_;
  std::function<void(double)> handle_;
  time_t delay_nsec_;
public:
  IntervalTimerState(double delay_sec, std::function<void(double)> f) 
  : handle_(f), delay_nsec_(delay_sec * 1.0e9) {
    // isolate thread
    running_ = true;
    pthread_create(&thread_, nullptr, RGLIntervalTimerThreadEntry, this);
  }

  ~IntervalTimerState() {
    running_ = false;
    pthread_join(thread_, NULL);
  }

  void Run() {
    using namespace std::chrono;
    auto last = high_resolution_clock::now();
    RGLSleep(delay_nsec_);
    double last_sec = (double)duration_cast<nanoseconds>(last.time_since_epoch()).count() / 1.0e9;
    double timer = 0;
    while (running_) {
        auto now = high_resolution_clock::now();
        auto time_span = duration_cast<duration<double>>(now - last);
        double duration_sec = time_span.count();
        time_t duration = (time_t)(duration_sec * 1.0e9);
        long delta = (long)delay_nsec_ - (long)duration;
        last = now;
        long corrected = delay_nsec_ + delta;
        if (corrected < 10000) {
            corrected = 10000;
        }
        auto dur = duration_cast<nanoseconds>(now.time_since_epoch()).count();
        double now_sec = static_cast<double>(dur) / 1.0e9;
        timer += now_sec - last_sec;
        handle_(timer);
        last_sec = now_sec;
        RGLSleep(corrected);
    }
  }
};

IntervalTimer::IntervalTimer(double delay_sec, std::function<void(double)> f)
    : state_(new IntervalTimerState(delay_sec, f)) {}

IntervalTimer::~IntervalTimer() {
  delete state_;
}

}  // namespace rigel

void *RGLIntervalTimerThreadEntry(void *state) {
  static_cast<rigel::IntervalTimerState *>(state)->Run();
  return 0;
}
