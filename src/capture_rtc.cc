
#include <random>

#include "capture_rtc.h"
#include "logging.inc"

#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_base/time_utils.h"

namespace rigel {

void VideoCapturer::Initialize() {
  constexpr int width = 960;
  constexpr int height = 544;
  // frame buffer
  buffer_ = webrtc::I420Buffer::Create(width, height);
  webrtc::I420Buffer::SetBlack(buffer_.get());
  // generate frame
  int64_t timems = rtc::TimeMillis();
  webrtc::VideoFrame frame(buffer_, 0, timems, webrtc::kVideoRotation_0);
  frame.set_ntp_time_ms(0);
  OnFrame(frame);
}

void VideoCapturer::OnRenderFrame(const char *map, int w, int h, int r) {
  int width = buffer_->width();
  int height = buffer_->height();
  const uint8_t *data = reinterpret_cast<const uint8_t *>(map);
  webrtc::I420Buffer *buffer = buffer_.get();
  libyuv::ConvertToI420(
      data, CalcBufferSize(webrtc::VideoType::kARGB, width, height),
      buffer->MutableDataY(), buffer->StrideY(),
      buffer->MutableDataU(), buffer->StrideU(),
      buffer->MutableDataV(), buffer->StrideV(),
      0, 0,
      width, height,
      buffer->width(), buffer->height(),
      libyuv::kRotate0,
      libyuv::FOURCC_ABGR);
  // generate frame
  int64_t timems = rtc::TimeMillis();
  webrtc::VideoFrame frame(buffer_, 0, timems, webrtc::kVideoRotation_0);
  frame.set_ntp_time_ms(0);
  OnFrame(frame);
}

}  // namespace rigel
