
#include "render_instance.h"

namespace rigel {

void RenderInstance::StartRendering() {
  renderer_ = std::unique_ptr<GraphicsRenderer>(new GraphicsRenderer());
  auto *timer = new IntervalTimer(1.0 / 60, [=](double time_sec) {
    this->OnTick(time_sec);
  });
  timer_ = std::unique_ptr<IntervalTimer>(timer);
  renderer_->Render(0, 0, 0);
}

void RenderInstance::StopRendering() {
  timer_ = nullptr;
}

void RenderInstance::OnTick(double time_sec) {
  renderer_->Capture([=](const char *v, int w, int h, int r) {
    this->sink_->OnRenderFrame(v, w, h, r);
  });
  renderer_->Render(time_sec, 0, 0);
}

}  // namespace rigel
