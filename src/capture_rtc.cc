
#include <random>

#include "capture_rtc.h"
#include "logging.inc"

#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace rigel {

cricket::CaptureState VideoCapturer::Start(
    const cricket::VideoFormat& capture_format) {
  if (capture_state() == cricket::CS_RUNNING) {
    return capture_state();
  }
  SetCaptureFormat(&capture_format);
  return cricket::CS_RUNNING;
}

void VideoCapturer::Stop() {
  if (capture_state() == cricket::CS_STOPPED) {
    return;
  }
  SetCaptureFormat(nullptr);
  SetCaptureState(cricket::CS_STOPPED);
}

bool VideoCapturer::GetPreferredFourccs(std::vector<uint32_t>* fourccs) {
  if (!fourccs) return false;
  fourccs->push_back(cricket::FOURCC_I420);
  return true;
}

bool VideoCapturer::GetBestCaptureFormat(const cricket::VideoFormat& desired,
    cricket::VideoFormat* best_format) {
  if (!best_format) return false;
  best_format->width = desired.width;
  best_format->height = desired.height;
  best_format->fourcc = cricket::FOURCC_I420;
  best_format->interval = desired.interval;
  return true;
}

static void YUVRect(webrtc::I420Buffer *buffer,
    int value_y, int value_u, int value_v) {
  libyuv::I420Rect(buffer->MutableDataY(), buffer->StrideY(),
      buffer->MutableDataU(), buffer->StrideU(),
      buffer->MutableDataV(), buffer->StrideV(), 0, 0,
      buffer->width(), buffer->height(), value_y, value_u, value_v);
}

static std::tuple<int, int, int> RandomYUV() {
  std::vector<std::tuple<int, int, int>> colors = {
    {149, 43, 21},    // green
    {76, 84, 255},    // red
    {29, 255, 107},   // blue
    {105, 212, 234},  // purple
    {225, 0, 148},    // yellow
    {178, 171, 0}     // cyan
  };
  std::random_device random_device;
  std::mt19937 engine{random_device()};
  std::uniform_int_distribution<int> dist(0, colors.size() - 1);
  return colors[dist(engine)];
}

void VideoCapturer::Initialize() {
  constexpr int width = 960;
  constexpr int height = 544;
  RGL_WARN("VideoCapturer::Initialize()");
  SetCaptureState(
      Start(cricket::VideoFormat(width, height,
          cricket::VideoFormat::FpsToInterval(60), cricket::FOURCC_I420)));
  // frame buffer
  buffer_ = webrtc::I420Buffer::Create(width, height);
  // fill random color
  // int y, u, v;
  // std::tie(y, u, v) = RandomYUV();
  // YUVRect(buffer.get(), y, u, v);
  webrtc::I420Buffer::SetBlack(buffer_.get());
  // generate frame
  int64_t timems = rtc::TimeMillis();
  webrtc::VideoFrame frame(buffer_, 0, timems, webrtc::kVideoRotation_0);
  frame.set_ntp_time_ms(0);
  OnFrame(frame, width, height);
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
  OnFrame(frame, width, height);
}

}  // namespace rigel
