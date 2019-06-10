
#include <string>

#include "signaling_websocket.h"
#include "logging.inc"

namespace rigel {

namespace {
class SignalingWebSocketControl : public SignalingControlInterface {
 public:
  explicit SignalingWebSocketControl(WebSocketInterface *ws) : ws_(ws) {}
  // Signaling Control
  void SendMessage(const std::string &message) override {
    ws_->SendMessage(message);
  }
 private:
  WebSocketInterface *ws_;
};
}  // unnamed namespace

void SignalingStrategyWebSocket::Initialize() {
  RGLLaunchWebSocketSession(hostname_, port_, path_, this);
}

void SignalingStrategyWebSocket::OnHandshake(WebSocketInterface *ws)  {
  RGL_INFO("WebSocket connection established. creating instance");
  std::unique_ptr<SignalingControlInterface> control(
    new SignalingWebSocketControl(ws));
  instance_ = factory_->CreateInstance(std::move(control));
  instance_->Initialize();
}

void SignalingStrategyWebSocket::OnReceiveMessage(
    WebSocketInterface *connection,
    const std::string &message) {
  if (!instance_) {
    RGL_WARN("instance not found");
    return;
  }
  instance_->ReceiveMessage(message);
}

void SignalingStrategyWebSocket::OnRelease(WebSocketInterface *connection) {
  RGL_INFO("SignalingStrategyWebSocket::OnRelease");
  instance_ = nullptr;
}

}  // namespace rigel
