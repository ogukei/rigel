
#ifndef RIGEL_BASE_MESSAGE_SIGNALING_H_
#define RIGEL_BASE_MESSAGE_SIGNALING_H_

#include <vector>
#include <string>

#include "message_storage.h"
#include "ice_candidate.h"

namespace rigel {

struct SignalingMessageIncomingSink {
  virtual void OnStart(const std::string &source) = 0;
  virtual void OnClose(const std::string &source) = 0;
  virtual void OnAcceptAnswer(const std::string &source,
      const std::string &sdp) = 0;
  virtual void OnICECandidates(const std::string &source,
      const std::vector<ICECandidate> &candidates) = 0;
  virtual void OnAcquire(const std::string &source) = 0;
};

struct SignalingMessageOutgoingSink {
  virtual void SendMessage(const std::string &message) = 0;
};

struct SignalingMessageInterface {
  virtual void SendOffer(const std::string &destination,
      const std::string &sdp) = 0;
  virtual void SendICECandidates(const std::string &destination,
      const std::vector<ICECandidate> &candidates) = 0;
};

class SignalingMessageDispatcher : public SignalingMessageInterface {
 public:
  explicit SignalingMessageDispatcher(SignalingMessageIncomingSink *incoming,
      SignalingMessageOutgoingSink *outgoing);

  void DispatchMessage(const std::string &message);

  // SignalingMessageInterface
  void SendOffer(const std::string &destination,
      const std::string &sdp) override;
  void SendICECandidates(const std::string &destination,
      const std::vector<ICECandidate> &candidates) override;
 private:
  RMIStorage<SignalingMessageIncomingSink,
      const std::string &, const std::string &> storage_;
  SignalingMessageIncomingSink *incoming_;
  SignalingMessageOutgoingSink *outgoing_;
};

}  // namespace rigel

#endif  // RIGEL_BASE_MESSAGE_SIGNALING_H_
