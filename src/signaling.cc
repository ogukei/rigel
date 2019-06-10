
#include "signaling.h"
#include "signaling_websocket.h"
#include "instance.h"

namespace rigel {

using DefaultSignalingStrategy = ::rigel::SignalingStrategyWebSocket;

SignalingContext::SignalingContext(const std::string &hostname,
                                   const std::string &port,
                                   const std::string &path)
    : strategy_(new DefaultSignalingStrategy(this, hostname, port, path)) {}

SignalingContext::~SignalingContext() {}

void SignalingContext::Run() {
  strategy_->Initialize();
}

std::unique_ptr<SignalingInstanceInterface> SignalingContext::CreateInstance(
    std::unique_ptr<SignalingControlInterface> control) {
  return std::unique_ptr<SignalingInstanceInterface>(
    new SignalingInstance(std::move(control)));
}

}  // namespace rigel
