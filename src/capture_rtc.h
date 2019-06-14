
#ifndef RIGEL_RTC_CAPTURE_H_
#define RIGEL_RTC_CAPTURE_H_

#include <memory>

#include "media/base/video_broadcaster.h"
#include "api/video/i420_buffer.h"
#include "render.h"

namespace rigel {

class VideoCapturer : public rtc::VideoBroadcaster,
    public RenderInstanceSink {
 public:
  VideoCapturer() = default;
  ~VideoCapturer() override = default;

  // VideoCapturer
  void Initialize();

  // RenderInstanceSink
  void OnRenderFrame(const char *v, int w, int h, int r) override;

 private:
  rtc::scoped_refptr<webrtc::I420Buffer> buffer_;
};

}  // namespace rigel

#endif  // RIGEL_RTC_CAPTURE_H_

