
#ifndef RIGEL_BASE_SIGNALING_WEBSOCKET_H_
#define RIGEL_BASE_SIGNALING_WEBSOCKET_H_

#include <string>
#include <memory>

#include "signaling.h"
#include "websocket.h"

namespace rigel {

class SignalingStrategyWebSocket : public SignalingStrategy,
    WebSocketSink {
 public:
  explicit SignalingStrategyWebSocket(
      SignalingInstanceFactoryInterface *factory,
      const std::string &hostname,
      const std::string &port,
      const std::string &path)
      : factory_(factory), hostname_(hostname), port_(port), path_(path) {}

  explicit SignalingStrategyWebSocket(
      const SignalingStrategyWebSocket &) = delete;

  ~SignalingStrategyWebSocket() override = default;

  // SignalingStrategy
  void Initialize() override;

  // WebSocketSink
  void OnHandshake(WebSocketInterface *ws) override;
  void OnReceiveMessage(WebSocketInterface *connection,
      const std::string &message) override;
  void OnRelease(WebSocketInterface *connection) override;

 private:
  SignalingInstanceFactoryInterface *factory_;
  std::string hostname_;
  std::string port_;
  std::string path_;
  std::unique_ptr<SignalingInstanceInterface> instance_;
};

}  // namespace rigel

#endif  // RIGEL_BASE_SIGNALING_WEBSOCKET_H_
