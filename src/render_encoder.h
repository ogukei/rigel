
#ifndef RIGEL_GRAPHICS_RENDER_ENCODER_H_
#define RIGEL_GRAPHICS_RENDER_ENCODER_H_

#include <functional>

// @see https://webrtc.googlesource.com/src/+/branch-heads/m75/modules/video_coding/codecs/h264/h264_encoder_impl.h
#include <memory>
#include <vector>
#include "api/video/i420_buffer.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/utility/quality_scaler.h"
#include "third_party/openh264/src/codec/api/svc/codec_app_def.h"
#include "video/encoder_bitrate_adjuster.h"

class NvEncoderCuda;

namespace rigel {

class RenderContextCuda;

// @see https://webrtc.googlesource.com/src/+/branch-heads/m75/modules/video_coding/codecs/h264/h264_encoder_impl.h
class RenderH264Encoder : public webrtc::H264Encoder {
 public:
  explicit RenderH264Encoder(RenderContextCuda *cuda);
  ~RenderH264Encoder() override;
  // |max_payload_size| is ignored.
  // The following members of |codec_settings| are used. The rest are ignored.
  // - codecType (must be kVideoCodecH264)
  // - targetBitrate
  // - maxFramerate
  // - width
  // - height
  int32_t InitEncode(const webrtc::VideoCodec* codec_settings,
                     int32_t number_of_cores,
                     size_t max_payload_size) override;
  int32_t Release() override;
  int32_t RegisterEncodeCompleteCallback(
      webrtc::EncodedImageCallback* callback) override;
  void SetRates(const RateControlParameters& parameters) override;
  // The result of encoding - an EncodedImage and RTPFragmentationHeader - are
  // passed to the encode complete callback.
  int32_t Encode(const webrtc::VideoFrame& frame,
                 const std::vector<webrtc::VideoFrameType>* frame_types) override;
  EncoderInfo GetEncoderInfo() const override;

 private:
  RenderContextCuda *cuda_;
  NvEncoderCuda *encoder_;
  // @see https://webrtc.googlesource.com/src/+/69202b2a57b8b7f7046dc26930aafd6f779a152e/api/video_codecs/video_encoder.h
  webrtc::EncodedImageCallback *encoded_image_callback_;
  webrtc::EncodedImage encoded_image_;
  std::vector<std::vector<uint8_t>> packets_;
  webrtc::H264BitstreamParser h264_bitstream_parser_;
  uint32_t target_bitrate_;
  bool should_initialize_;
  bool is_sending_;
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENCODER_H_
