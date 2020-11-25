
#include "render_encoder_factory.h"
#include "render_encoder.h"
#include "render_context_cuda.h"

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

// @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/modules/video_coding/codecs/h264/h264.cc
static SdpVideoFormat CreateH264Format(H264::Profile profile,
                                H264::Level level,
                                const std::string& packetization_mode) {
  const absl::optional<std::string> profile_string =
      H264::ProfileLevelIdToString(H264::ProfileLevelId(profile, level));
  RTC_CHECK(profile_string);
  return SdpVideoFormat(
      cricket::kH264CodecName,
      {{cricket::kH264FmtpProfileLevelId, *profile_string},
       {cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
       {cricket::kH264FmtpPacketizationMode, packetization_mode}});
}

namespace rigel {

class RigelVideoEncoderFactory : public VideoEncoderFactory {
 public:
  RigelVideoEncoderFactory();
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

RigelVideoEncoderFactory::RigelVideoEncoderFactory() : cuda_(new RenderContextCuda()) {
  // @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/modules/video_coding/codecs/h264/h264.cc
  supported_formats_.push_back(CreateH264Format(H264::kProfileBaseline, H264::kLevel5, "0"));
  supported_formats_.push_back(CreateH264Format(H264::kProfileConstrainedBaseline, H264::kLevel5, "0"));
}

RigelVideoEncoderFactory::~RigelVideoEncoderFactory() {
  delete cuda_;
  cuda_ = nullptr;
}

VideoEncoderFactory::CodecInfo RigelVideoEncoderFactory::QueryVideoEncoder(
      const SdpVideoFormat& format) const {
  RTC_DCHECK(IsFormatSupported(supported_formats_, format));
  CodecInfo info;
  info.has_internal_source = false;
  info.is_hardware_accelerated = false;
  return info;
}

std::unique_ptr<VideoEncoder> RigelVideoEncoderFactory::CreateVideoEncoder(
      const SdpVideoFormat& format) {
  RTC_DCHECK(IsFormatSupported(supported_formats_, format));
  return absl::make_unique<RenderH264Encoder>(cuda_);
}

//
std::unique_ptr<webrtc::VideoEncoderFactory> CreateRigelVideoEncoderFactory() {
  return absl::make_unique<RigelVideoEncoderFactory>();
}

}  // namespace rigel

