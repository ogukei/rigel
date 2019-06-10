
#ifndef RIGEL_BASE_INSTANCE_H_
#define RIGEL_BASE_INSTANCE_H_

#include <string>
#include <memory>
#include <unordered_map>

#include "signaling_strategy.h"
#include "signaling_sink.h"
#include "channel.h"
#include "message_signaling.h"
#include "render.h"

namespace rigel {

class RTCInstance;

class SignalingInstance: public SignalingInstanceInterface,
    SignalingMessageIncomingSink,
    SignalingMessageOutgoingSink {
 public:
  explicit SignalingInstance(
      std::unique_ptr<SignalingControlInterface> control);
  explicit SignalingInstance(const SignalingInstance &) = delete;
  virtual ~SignalingInstance();

  // SignalingInstanceInterface
  void Initialize() override;
  void ReceiveMessage(const std::string &message) override;

 private:
  RTCInstance *rtc_;
  std::unique_ptr<SignalingControlInterface> control_;
  std::unordered_map<std::string,
      std::unique_ptr<PeerChannelInterface>> channel_map_;
  std::unique_ptr<SignalingMessageDispatcher> message_dispatcher_;
  std::unique_ptr<RenderContext> render_context_;

  // SignalingMessageIncomingSink
  void OnStart(const std::string &source) override;
  void OnClose(const std::string &source) override;
  void OnAcquire(const std::string &source) override;
  void OnAcceptAnswer(const std::string &source,
      const std::string &sdp) override;
  void OnICECandidates(const std::string &source,
      const std::vector<ICECandidate> &candidates) override;
  // SignalingMessageOutgoingSink
  void SendMessage(const std::string &message) override;
};

}  // namespace rigel

#endif  // RIGEL_BASE_INSTANCE_H_
