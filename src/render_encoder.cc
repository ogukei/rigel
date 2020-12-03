
#include "render_encoder.h"
#include "logging.inc"
#include "render_nv_encoder_cuda.h"
#include "render_context_cuda.h"
#include "capture_frame.h"
#include "render_encode_buffer.h"

#include <cuda.h>
#include "common_video/h264/h264_common.h"
#include "rtc_base/bit_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "api/video/video_frame_type.h"

using namespace webrtc;

namespace rigel {

RenderH264Encoder::RenderH264Encoder(RenderContextCuda *cuda) : 
cuda_(cuda), encoded_image_callback_(nullptr), encoder_(nullptr),
target_bitrate_(0), should_initialize_(false), is_sending_(false) {
  
}

RenderH264Encoder::~RenderH264Encoder() {
  Release();
}

int32_t RenderH264Encoder::InitEncode(const VideoCodec* codec_settings,
                    int32_t number_of_cores,
                    size_t max_payload_size) {
  if (encoder_ != nullptr) {
    Release();
  }
  encoded_image_ = {};
  // VideoCodec
  // @see https://chromium.googlesource.com/external/webrtc/+/ad34dbe934/webrtc/media/base/codec.h
  auto width = codec_settings->width;
  auto height = codec_settings->height;
  // Initialize encoded image. Default buffer size: size of unencoded data.
  // @see https://chromium.googlesource.com/external/webrtc/+/branch-heads/m75/modules/video_coding/codecs/h264/h264_encoder_impl.cc
  // @see https://chromium.googlesource.com/external/webrtc/+/ad34dbe934/webrtc/modules/video_coding/codecs/h264/h264_encoder_impl.cc#224
  encoded_image_._completeFrame = true;
  encoded_image_._encodedWidth = width;
  encoded_image_._encodedHeight = height;
  encoded_image_.set_size(0);
  // @see http://developer.download.nvidia.com/assets/cuda/files/NvEncodeAPI_v.6.0.pdf
  is_sending_ = false;
  should_initialize_ = true;
  encoder_ = new NvEncoderCuda(cuda_->Context(), width, height, NV_ENC_BUFFER_FORMAT_ABGR);
  try {
    NV_ENC_INITIALIZE_PARAMS initialize_params = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encode_config = { NV_ENC_CONFIG_VER };
    initialize_params.encodeConfig = &encode_config;
    encoder_->CreateDefaultEncoderParams(&initialize_params, 
        NV_ENC_CODEC_H264_GUID, NV_ENC_PRESET_P3_GUID, NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY);
    {
      encode_config.rcParams.averageBitRate = 200000;
      initialize_params.frameRateNum = 120;
    }
    encoder_->CreateEncoder(&initialize_params);
  } catch (const NVENCException &e) {
    RGL_WARN("NvEncoderCuda initialization error");
    RGL_WARN(e.what());
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t RenderH264Encoder::Release() {
  is_sending_ = false;
  if (encoder_ != nullptr) {
    encoder_->EndEncode(packets_);
    encoder_->DestroyEncoder();
    delete encoder_;
    encoder_ = nullptr;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t RenderH264Encoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_image_callback_ = callback;               
  return WEBRTC_VIDEO_CODEC_OK;
}

void RenderH264Encoder::SetRates(const RateControlParameters& parameters) {
  if (!is_sending_) return;
  if (encoder_ == nullptr) return;
  uint32_t desired = parameters.bitrate.get_sum_bps();
  if (desired == target_bitrate_) return;
  target_bitrate_ = desired;
  RGL_INFO("SetRates");
  RGL_INFO(target_bitrate_);
  try {
    NV_ENC_RECONFIGURE_PARAMS reconfigure_params = { NV_ENC_RECONFIGURE_PARAMS_VER };
    NV_ENC_CONFIG encode_config = { NV_ENC_CONFIG_VER };
    reconfigure_params.reInitEncodeParams.encodeConfig = &encode_config;
    encoder_->GetInitializeParams(&reconfigure_params.reInitEncodeParams);
    encode_config.rcParams.averageBitRate = target_bitrate_;
    encoder_->Reconfigure(&reconfigure_params);
  } catch (const NVENCException &e) {
    RGL_WARN("NvEncoderCuda reconfigure error");
    RGL_WARN(e.what());
    return;
  }
}

int32_t RenderH264Encoder::Encode(const VideoFrame& frame,
                const std::vector<VideoFrameType>* frame_types) {

  auto frame_buffer = frame.video_frame_buffer();
  auto width = frame_buffer->width();
  auto height = frame_buffer->height();
  auto *native_buffer = static_cast<NativeVideoFrameBuffer *>(frame_buffer.get());
  if (native_buffer->DevicePointer() == nullptr) return WEBRTC_VIDEO_CODEC_ERROR;
  // initialize
  if (should_initialize_) {
    should_initialize_ = false;
    encoder_->RegisterInputFrame(native_buffer->DevicePointer(), native_buffer->Pitch());
    is_sending_ = true;
  }
  if (!is_sending_) return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  // encode
  encoder_->EncodeFrame(packets_);
  // output
  for (std::vector<uint8_t> &encoded_output_buffer: packets_) {
    // @see https://webrtc.googlesource.com/src/+/69202b2a57b8b7f7046dc26930aafd6f779a152e/api/video/encoded_image.h
    auto data = NonCopyEncodedImageBuffer::Create(encoded_output_buffer.data(), encoded_output_buffer.size());
    encoded_image_.SetEncodedData(data);
    encoded_image_._encodedWidth = frame_buffer->width();
    encoded_image_._encodedHeight = frame_buffer->height();
    encoded_image_.SetTimestamp(frame.timestamp());
    encoded_image_.ntp_time_ms_ = frame.ntp_time_ms();
    encoded_image_.capture_time_ms_ = frame.render_time_ms();
    encoded_image_.rotation_ = frame.rotation();
    encoded_image_.qp_ = 5;
    encoded_image_.content_type_ = VideoContentType::UNSPECIFIED;
    encoded_image_.timing_.flags = VideoSendTiming::kInvalid;
    encoded_image_.SetColorSpace(frame.color_space());
    encoded_image_.SetSpatialIndex(0);
    // obtain QP from bitstream
    h264_bitstream_parser_.ParseBitstream(encoded_image_.data(), encoded_image_.size());
    h264_bitstream_parser_.GetLastSliceQp(&encoded_image_.qp_);
    // encoded image
    CodecSpecificInfo codec_specific;
    codec_specific.codecType = kVideoCodecH264;
    codec_specific.codecSpecific.H264.packetization_mode = H264PacketizationMode::NonInterleaved;
    // @see https://webrtc.googlesource.com/src/+/69202b2a57b8b7f7046dc26930aafd6f779a152e/api/video_codecs/video_encoder.h
    encoded_image_callback_->OnEncodedImage(encoded_image_, &codec_specific);
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

H264Encoder::EncoderInfo RenderH264Encoder::GetEncoderInfo() const {
  EncoderInfo info;
  info.supports_native_handle = true;
  info.implementation_name = "RIGEL_NVENC_H264";
  info.scaling_settings = ScalingSettings(ScalingSettings::kOff);
  info.is_hardware_accelerated = true;
  info.has_internal_source = false;
  return info;
}

}  // namespace rigel
