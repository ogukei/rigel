
#ifndef RIGEL_GRAPHICS_RENDER_INSTANCE_H_
#define RIGEL_GRAPHICS_RENDER_INSTANCE_H_

#include <memory>

#include "render.h"
#include "render_timer.h"
#include "render_engine.h"

namespace rigel {

class RenderInstance : public RenderInstanceInterface {
 public:
  explicit RenderInstance(RenderInstanceSink *sink) : sink_(sink) {}

  void StartRendering() override;
  void StopRendering() override;
 private:
  RenderInstanceSink *sink_;
  std::unique_ptr<IntervalTimer> timer_;
  std::unique_ptr<GraphicsRenderer> renderer_;

  void OnTick(double time_sec);
};


}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_INSTANCE_H_
