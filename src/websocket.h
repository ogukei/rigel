
#ifndef RIGEL_BASE_WEBSOCKET_H_
#define RIGEL_BASE_WEBSOCKET_H_

#include <string>
#include <memory>

namespace rigel {

struct WebSocketInterface {
  virtual void SendMessage(const std::string &message) = 0;
};

struct WebSocketSink {
  virtual void OnHandshake(WebSocketInterface *connection) = 0;
  virtual void OnReceiveMessage(WebSocketInterface *connection,
      const std::string &message) = 0;
  virtual void OnRelease(WebSocketInterface *connection) = 0;
};

struct WebSocketInstance {
  virtual ~WebSocketInstance() = default;
};

void RGLLaunchWebSocketSession(const std::string &host,
    const std::string &port,
    const std::string &path, WebSocketSink *sink);

std::unique_ptr<WebSocketInstance> RGLAsyncLaunchWebSocketSession(
    const std::string &host,
    const std::string &port,
    const std::string &path, WebSocketSink *sink);

}  // namespace rigel

#endif  // RIGEL_BASE_WEBSOCKET_H_
