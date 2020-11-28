
#include <random>

#include "capture_rtc.h"
#include "capture_frame.h"
#include "logging.inc"

#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/ref_counted_object.h"


namespace rigel {

void VideoCapturer::Initialize() {
  constexpr int width = 960;
  constexpr int height = 544;
  // frame buffer
  buffer_ = new rtc::RefCountedObject<NativeVideoFrameBuffer>(width, height);
}

void VideoCapturer::OnRenderFrame(const char *map, int w, int h, int r) {
  buffer_->Configure((void *)map, r);
  // generate frame
  auto frame = webrtc::VideoFrame::Builder()
      .set_video_frame_buffer(buffer_)
      .build();
  OnFrame(frame);
}

}  // namespace rigel
