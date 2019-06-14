
#include "render_timer.h"
#include "logging.inc"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <iostream>

extern "C" {
#include <time.h>
#include <pthread.h>
void *RGLIntervalTimerThreadEntry(void *state);
}

constexpr uint64_t NSEC_PER_SEC = 1000000000L;

static inline void RGLAbsoluteSleep(uint64_t nsec) {
  timespec t {
    static_cast<time_t>(nsec / NSEC_PER_SEC),
    static_cast<__syscall_slong_t>(nsec % NSEC_PER_SEC)
  };
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
}

static inline uint64_t RGLGetTime() {
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  uint64_t nsec = (static_cast<uint64_t>(time.tv_sec) * NSEC_PER_SEC) +
    static_cast<uint64_t>(time.tv_nsec);
  return nsec;
}

namespace rigel {
class IntervalTimerState {
 public:
  IntervalTimerState(double interval_sec,
      std::function<void(double)> f)
      : handle_(f), interval_(interval_sec * static_cast<double>(NSEC_PER_SEC)),
        frame_counter_(0), frame_count_since_(0) {
    // isolate thread
    running_ = true;
    pthread_create(&thread_, nullptr, RGLIntervalTimerThreadEntry, this);
  }

  ~IntervalTimerState() {
    running_ = false;
    pthread_join(thread_, NULL);
  }

  void Run() {
    since_ = RGLGetTime();
    frame_count_since_ = 0;
    while (running_) {
      uint64_t global_time = RGLGetTime();
      uint64_t local_time = global_time - since_;
      {
        // counts frames per second
        if (local_time > (frame_count_since_ + NSEC_PER_SEC)) {
          RGL_INFO(frame_counter_);
          frame_counter_ = 1;
          frame_count_since_ = local_time;
        } else {
          frame_counter_ += 1;
        }
        // processing
        handle_(local_time * 1.0e-9);
      }
      RGLAbsoluteSleep(global_time + interval_);
    }
  }

 private:
  pthread_t thread_;
  // internal state
  volatile bool running_;
  std::function<void(double)> handle_;
  int64_t interval_;
  uint64_t since_;

  int64_t frame_counter_;
  int64_t frame_count_since_;
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
