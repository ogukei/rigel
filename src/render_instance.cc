
#include "render_instance.h"
#include <boost/lockfree/queue.hpp>

namespace rigel {

struct RenderInstanceMessage {
  int x, y, z;
};

class RenderInstancePrivate {
 public:
  RenderInstancePrivate() : message_queue_(128), x_(0), y_(0), z_(0),
      is_initial_state_(true) {}

  void Update() {
    RenderInstanceMessage message;
    while (message_queue_.pop(message)) {
      x_ += message.x;
      y_ += message.y;
      z_ += message.z;
      is_initial_state_ = false;
    }
  }

  void Post(const RenderInstanceMessage &message) {
    message_queue_.push(message);
  }

  int GetX() const { return x_; }
  int GetY() const { return y_; }
  int GetZ() const { return z_; }
  bool IsInitialState() const { return is_initial_state_; }

 private:
  boost::lockfree::queue<RenderInstanceMessage> message_queue_;
  // internal state
  int x_;
  int y_;
  int z_;
  bool is_initial_state_;
};

RenderInstance::RenderInstance(RenderInstanceSink *sink)
    : sink_(sink), private_(new RenderInstancePrivate()) {}

RenderInstance::~RenderInstance() {
  delete private_;
  private_ = nullptr;
}

void RenderInstance::StartRendering() {
  renderer_ = std::unique_ptr<GraphicsRenderer>(new GraphicsRenderer());
  auto *timer = new IntervalTimer(1.0 / 30, [=](double time_sec) {
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
  private_->Update();
  if (private_->IsInitialState()) {
    renderer_->Render(time_sec, time_sec * 0.3, 0);
  } else {
    renderer_->Render(
      static_cast<float>(private_->GetX()) * 0.01,
      static_cast<float>(private_->GetY()) * 0.01,
      static_cast<float>(private_->GetZ()) * 0.01);
  }
}

void RenderInstance::InputXYAxis(int x, int y) {
  private_->Post(RenderInstanceMessage { x, y, 0 });
}

void RenderInstance::InputZAxis(int z) {
  private_->Post(RenderInstanceMessage { 0, 0, z });
}

}  // namespace rigel
