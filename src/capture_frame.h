

#ifndef RIGEL_RTC_CAPTURE_FRAME_H_
#define RIGEL_RTC_CAPTURE_FRAME_H_

#include <memory>

// @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/api/video/video_frame_buffer.h
#include "api/video/video_frame_buffer.h"
#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"

namespace rigel {

class NativeVideoFrameBuffer : public webrtc::VideoFrameBuffer {
 public:
  explicit NativeVideoFrameBuffer(int width, int height);

  // This function specifies in what pixel format the data is stored in.
  virtual webrtc::VideoFrameBuffer::Type type() const override {
    return webrtc::VideoFrameBuffer::Type::kNative;
  }
  // The resolution of the frame in pixels. For formats where some planes are
  // subsampled, this is the highest-resolution plane.
  virtual int width() const override { return width_; }
  virtual int height() const override { return height_; }

  // Returns a memory-backed frame buffer in I420 format. If the pixel data is
  // in another format, a conversion will take place. All implementations must
  // provide a fallback to I420 for compatibility with e.g. the internal WebRTC
  // software encoders.
  virtual rtc::scoped_refptr<webrtc::I420BufferInterface> ToI420() override;
 protected:
  ~NativeVideoFrameBuffer() override {}
 private:
  int width_;
  int height_;
};


}  // namespace rigel

#endif  // RIGEL_RTC_CAPTURE_FRAME_H_

