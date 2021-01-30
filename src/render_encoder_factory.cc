
#include "render_encoder_factory.h"
#include "render_encoder.h"
#include "render_context_cuda.h"
#include "render.h"
#include "logging.inc"

#include <memory>

// @see https://webrtc.googlesource.com/src/+/branch-heads/m75/api/video_codecs/builtin_video_encoder_factory.cc
#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "media/engine/encoder_simulcast_proxy.h"
#include "media/engine/internal_encoder_factory.h"
#include "rtc_base/checks.h"

// @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/modules/video_coding/codecs/h264/h264.cc
#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/h264_profile_level_id.h"
#include "media/base/media_constants.h"

// @see https://webrtc.googlesource.com/src/+/branch-heads/m75/modules/video_coding/codecs/h264/include/h264.h
#include "modules/video_coding/codecs/h264/include/h264.h"

using namespace webrtc;

namespace rigel {

// builtin_video_encoder_factory.cc
// @see https://webrtc.googlesource.com/src/+/branch-heads/m75/api/video_codecs/builtin_video_encoder_factory.cc
static bool IsFormatSupported(const std::vector<SdpVideoFormat>& supported_formats,
                       const SdpVideoFormat& format) {
  for (const SdpVideoFormat& supported_format : supported_formats) {
    if (cricket::IsSameCodec(format.name, format.parameters,
                             supported_format.name,
                             supported_format.parameters)) {
      return true;
    }
  }
  return false;
}


class RigelVideoEncoderFactory : public VideoEncoderFactory {
 public:
  RigelVideoEncoderFactory(RenderContext *render_context);
  virtual ~RigelVideoEncoderFactory();

  VideoEncoderFactory::CodecInfo QueryVideoEncoder(
      const SdpVideoFormat& format) const override;
  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
      const SdpVideoFormat& format) override;
  std::vector<SdpVideoFormat> GetSupportedFormats() const override {
    return supported_formats_;
  }
 private:
  std::vector<SdpVideoFormat> supported_formats_;
  RenderContextCuda *cuda_;
};

RigelVideoEncoderFactory::RigelVideoEncoderFactory(RenderContext *render_context) : cuda_(render_context->Cuda()) {
  // @see https://chromium.googlesource.com/external/webrtc/+/69202b2a57b8b7f7046dc26930aafd6f779a152e/media/engine/fake_webrtc_video_engine.cc
  // @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/modules/video_coding/codecs/h264/h264.cc
  supported_formats_.push_back(CreateH264Format(H264::kProfileBaseline, H264::kLevel3_1, "1"));
  supported_formats_.push_back(CreateH264Format(H264::kProfileBaseline, H264::kLevel3_1, "0"));
  supported_formats_.push_back(CreateH264Format(H264::kProfileConstrainedBaseline, H264::kLevel3_1, "1"));
  supported_formats_.push_back(CreateH264Format(H264::kProfileConstrainedBaseline, H264::kLevel3_1, "0"));
}

RigelVideoEncoderFactory::~RigelVideoEncoderFactory() {}

VideoEncoderFactory::CodecInfo RigelVideoEncoderFactory::QueryVideoEncoder(
      const SdpVideoFormat& format) const {
  CodecInfo info;
  info.has_internal_source = false;
  return info;
}

std::unique_ptr<VideoEncoder> RigelVideoEncoderFactory::CreateVideoEncoder(
      const SdpVideoFormat& format) {
  return absl::make_unique<RenderH264Encoder>(cuda_);
}

//
std::unique_ptr<webrtc::VideoEncoderFactory> CreateRigelVideoEncoderFactory(RenderContext *render_context) {
  return absl::make_unique<RigelVideoEncoderFactory>(render_context);
}

}  // namespace rigel

