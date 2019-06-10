
#ifndef RIGEL_RTC_OBSERVER_H_
#define RIGEL_RTC_OBSERVER_H_

#include "api/peer_connection_interface.h"
#include "api/jsep.h"

namespace rigel {

struct CreateSessionDescriptionSink {
  virtual void OnCreateSessionDescriptionSuccess(
      webrtc::SessionDescriptionInterface* desc) = 0;
};

struct SetSessionDescriptionSink {
  virtual void OnSetSessionDescriptionSuccess(webrtc::SdpType) = 0;
};

class CreateSessionDescriptionObserver :
    public webrtc::CreateSessionDescriptionObserver {
 public:
  explicit CreateSessionDescriptionObserver(
      CreateSessionDescriptionSink *sink) : sink_(sink) {}
  virtual ~CreateSessionDescriptionObserver() = default;

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
    sink_->OnCreateSessionDescriptionSuccess(desc);
  }

  void OnFailure(webrtc::RTCError error) override {}
 private:
  CreateSessionDescriptionSink *sink_;
};

class SetSessionDescriptionObserver :
    public webrtc::SetSessionDescriptionObserver {
 public:
  explicit SetSessionDescriptionObserver(
      SetSessionDescriptionSink *sink,
      webrtc::SdpType type) : sink_(sink), sdp_type_(type) {}
  virtual ~SetSessionDescriptionObserver() = default;

  void OnSuccess() override {
    sink_->OnSetSessionDescriptionSuccess(sdp_type_);
  }
  // See description in CreateSessionDescriptionObserver for OnFailure.
  void OnFailure(webrtc::RTCError error) override {}

 private:
  SetSessionDescriptionSink *sink_;
  webrtc::SdpType sdp_type_;
};

}  // namespace rigel

#endif  // RIGEL_RTC_OBSERVER_H_
