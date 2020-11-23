
#include "render_encoder.h"
#include "logging.inc"
#include "render_nv_encoder_cuda.h"

#include <cuda.h>

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

RenderEncoder::RenderEncoder() : cuda_(new CudaContextImpl()) {
  encoder_ = new NvEncoderCuda(cuda_->Context(), 1280, 720, NV_ENC_BUFFER_FORMAT_IYUV);
}

RenderEncoder::~RenderEncoder() {
  delete encoder_;
  encoder_ = nullptr;
  delete cuda_;
  cuda_ = nullptr;
}


}  // namespace rigel
