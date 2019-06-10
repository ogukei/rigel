
#ifndef RIGEL_RTC_CAPTURE_H_
#define RIGEL_RTC_CAPTURE_H_

#include <memory>

#include "media/base/videocapturer.h"
#include "api/video/i420_buffer.h"
#include "render.h"

namespace rigel {

class VideoCapturer : public cricket::VideoCapturer,
    public RenderInstanceSink {
 public:
  VideoCapturer() = default;
  ~VideoCapturer() override = default;

  bool GetBestCaptureFormat(const cricket::VideoFormat& desired,
      cricket::VideoFormat* best_format) override;

  // Start the video capturer with the specified capture format.
  // Parameter
  //   capture_format: The caller got this parameter by either calling
  //                   GetSupportedFormats() and selecting one of the supported
  //                   or calling GetBestCaptureFormat().
  // Return
  //   CS_STARTING:  The capturer is trying to start. Success or failure will
  //                 be notified via the |SignalStateChange| callback.
  //   CS_RUNNING:   if the capturer is started and capturing.
  //   CS_FAILED:    if the capturer failes to start..
  //   CS_NO_DEVICE: if the capturer has no device and fails to start.
  cricket::CaptureState Start(
      const cricket::VideoFormat& capture_format) override;

  void Stop() override;
  // Check if the video capturer is running.
  bool IsRunning() override {
    return capture_state() == cricket::CS_RUNNING;
  }
  bool IsScreencast() const override {
    return false;
  }
  // subclasses override this virtual method to provide a vector of fourccs, in
  // order of preference, that are expected by the media engine.
  bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override;

  // VideoCapturer
  void Initialize();

  // RenderInstanceSink
  void OnRenderFrame(const char *v, int w, int h, int r) override;

 private:
  rtc::scoped_refptr<webrtc::I420Buffer> buffer_;
};

}  // namespace rigel

#endif  // RIGEL_RTC_CAPTURE_H_

