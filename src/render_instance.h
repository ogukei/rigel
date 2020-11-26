
#ifndef RIGEL_GRAPHICS_RENDER_INSTANCE_H_
#define RIGEL_GRAPHICS_RENDER_INSTANCE_H_

#include <memory>

#include "render.h"
#include "render_timer.h"
#include "render_engine.h"

namespace rigel {

class RenderInstancePrivate;
class RenderContext;

class RenderInstance : public RenderInstanceInterface {
 public:
  explicit RenderInstance(RenderInstanceSink *sink, RenderContext *context);
  ~RenderInstance();

  void StartRendering() override;
  void StopRendering() override;
  void InputXYAxis(int x, int y) override;
  void InputZAxis(int z) override;
 private:
  RenderContext *context_;
  RenderInstanceSink *sink_;
  std::unique_ptr<IntervalTimer> timer_;
  std::unique_ptr<GraphicsRenderer> renderer_;
  RenderInstancePrivate *private_;
  void OnTick(double time_sec);
};


}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_INSTANCE_H_
