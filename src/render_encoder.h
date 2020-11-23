
#ifndef RIGEL_GRAPHICS_RENDER_ENCODER_H_
#define RIGEL_GRAPHICS_RENDER_ENCODER_H_

#include <functional>

class NvEncoderCuda;

namespace rigel {

class CudaContextImpl;

class RenderEncoder {
 private:
  CudaContextImpl *cuda_;
  NvEncoderCuda *encoder_;
 public:
  RenderEncoder();
  ~RenderEncoder();
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENCODER_H_
