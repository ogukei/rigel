
#ifndef RIGEL_GRAPHICS_RENDER_CONTEXT_CUDA_H_
#define RIGEL_GRAPHICS_RENDER_CONTEXT_CUDA_H_

#include <functional>
#include <cuda.h>

namespace rigel {

class RenderContextCuda {
 public:
  RenderContextCuda();
  ~RenderContextCuda();

  CUcontext Context() const { return context_; }
 private:
  int n_gpu_;
  CUdevice device_;
  char device_name_[80];
  CUcontext context_;
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_CONTEXT_CUDA_H_
