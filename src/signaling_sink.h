
#ifndef RIGEL_BASE_SIGNALING_SINK_H_
#define RIGEL_BASE_SIGNALING_SINK_H_

#include <string>
#include <memory>

namespace rigel {

struct SignalingControlInterface {
  virtual ~SignalingControlInterface() = default;
  virtual void SendMessage(const std::string &message) = 0;
};

struct SignalingInstanceInterface {
  virtual ~SignalingInstanceInterface() = default;
  virtual void Initialize() = 0;
  virtual void ReceiveMessage(const std::string &message) = 0;
};

struct SignalingInstanceFactoryInterface {
  virtual std::unique_ptr<SignalingInstanceInterface> CreateInstance(
      std::unique_ptr<SignalingControlInterface> control) = 0;
};

}  // namespace rigel

#endif  // RIGEL_BASE_SIGNALING_SINK_H_
