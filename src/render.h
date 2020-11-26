
#ifndef RIGEL_GRAPHICS_RENDER_H_
#define RIGEL_GRAPHICS_RENDER_H_

#include <memory>

namespace rigel {

struct RenderInstanceSink {
  virtual void OnRenderFrame(const char *v, int w, int h, int r) = 0;
};

struct RenderInstanceInterface {
  virtual ~RenderInstanceInterface() = default;
  virtual void StartRendering() = 0;
  virtual void StopRendering() = 0;
  virtual void InputXYAxis(int x, int y) = 0;
  virtual void InputZAxis(int z) = 0;
};

struct RenderInstanceFactoryInterface {
  virtual ~RenderInstanceFactoryInterface() = default;
  virtual std::unique_ptr<RenderInstanceInterface> CreateInstance(
      RenderInstanceSink *sink) = 0;
};

class RenderContextCuda;

class RenderContext : public RenderInstanceFactoryInterface {
 public:
  RenderContext();
  virtual ~RenderContext();

  explicit RenderContext(const RenderContext &) = delete;
  std::unique_ptr<RenderInstanceInterface> CreateInstance(
      RenderInstanceSink *sink) override;
  
  RenderContextCuda *Cuda() const { return cuda_; }

 private:
  RenderContextCuda *cuda_;
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_H_
