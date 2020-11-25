
#include "render_context_cuda.h"
#include "logging.inc"

namespace rigel {

RenderContextCuda::RenderContextCuda() : n_gpu_(0), device_(0), context_(nullptr) {
  // @see https://github.com/NVIDIA/video-sdk-samples/blob/cf5b2cdbbb0c0869261041289fbadc188ebdc6c0/Samples/AppEncode/AppEncCuda/AppEncCuda.cpp
  cuInit(0);
  cuDeviceGetCount(&n_gpu_);
  cuDeviceGet(&device_, 0);
  cuDeviceGetName(device_name_, sizeof(device_name_), device_);
  RGL_INFO("Using CUDA device");
  RGL_INFO(device_name_);
  cuCtxCreate(&context_, 0, device_);
}

RenderContextCuda::~RenderContextCuda() {
  cuCtxDestroy(context_);
}

}  // namespace rigel
