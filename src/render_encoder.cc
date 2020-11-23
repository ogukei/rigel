
#include "render_encoder.h"
#include "logging.inc"
#include "render_nv_encoder_cuda.h"

#include <cuda.h>

using namespace webrtc;

namespace rigel {

class CudaContextImpl {
 public:
  CudaContextImpl();
  ~CudaContextImpl();

  CUcontext Context() const { return context_; }
 private:
  int n_gpu_;
  CUdevice device_;
  char device_name_[80];
  CUcontext context_;
};

CudaContextImpl::CudaContextImpl() : n_gpu_(0), device_(0), context_(nullptr) {
  // @see https://github.com/NVIDIA/video-sdk-samples/blob/cf5b2cdbbb0c0869261041289fbadc188ebdc6c0/Samples/AppEncode/AppEncCuda/AppEncCuda.cpp
  cuInit(0);
  cuDeviceGetCount(&n_gpu_);
  cuDeviceGet(&device_, 0);
  cuDeviceGetName(device_name_, sizeof(device_name_), device_);
  RGL_INFO("Using CUDA device");
  RGL_INFO(device_name_);
  cuCtxCreate(&context_, 0, device_);
}

CudaContextImpl::~CudaContextImpl() {
  cuCtxDestroy(context_);
}

//

RenderH264Encoder::RenderH264Encoder() : cuda_(new CudaContextImpl()) {
  
}

RenderH264Encoder::~RenderH264Encoder() {
  delete cuda_;
  cuda_ = nullptr;
  Release();
}

int32_t RenderH264Encoder::InitEncode(const VideoCodec* codec_settings,
                    int32_t number_of_cores,
                    size_t max_payload_size) {
  encoder_ = new NvEncoderCuda(cuda_->Context(), 1280, 720, NV_ENC_BUFFER_FORMAT_IYUV);
  return 0;
}

int32_t RenderH264Encoder::Release() {
  // TODO:      
  delete encoder_;
  encoder_ = nullptr;            
  return 0;
}

int32_t RenderH264Encoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  // TODO:                  
  return 0;
}

void RenderH264Encoder::SetRates(const RateControlParameters& parameters) {
  // TODO:                  
  
}
// The result of encoding - an EncodedImage and RTPFragmentationHeader - are
// passed to the encode complete callback.
int32_t RenderH264Encoder::Encode(const VideoFrame& frame,
                const std::vector<VideoFrameType>* frame_types) {
  // TODO:                  
  return 0;
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
