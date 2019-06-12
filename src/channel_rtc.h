
#ifndef RIGEL_RTC_CHANNEL_H_
#define RIGEL_RTC_CHANNEL_H_

#include <vector>

#include "api/peer_connection_interface.h"
#include "api/data_channel_interface.h"

#include "channel.h"
#include "message_signaling.h"
#include "observer_rtc.h"
#include "ice_candidate.h"
#include "capture_rtc.h"
#include "capture_track_source.h"
#include "render_instance.h"

namespace rigel {

class RTCPeerChannel : public PeerChannelInterface,
    public CreateSessionDescriptionSink,
    public SetSessionDescriptionSink,
    public webrtc::PeerConnectionObserver,
    public webrtc::DataChannelObserver {
 public:
  explicit RTCPeerChannel(const std::string &identifier,
      SignalingMessageInterface *messaging);

  ~RTCPeerChannel() override;

  void Initialize(rtc::scoped_refptr<webrtc::PeerConnectionInterface> &&,
      rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> &,
      RenderInstanceFactoryInterface *);

  void OnCreateSessionDescriptionSuccess(
      webrtc::SessionDescriptionInterface* desc) override;

  void OnSetSessionDescriptionSuccess(webrtc::SdpType sdp_type) override;

  // PeerChannelInterface
  void Offer() override;
  void AcceptAnswer(const std::string &answer) override;
  void ReceiveICECandidates(
      const std::vector<ICECandidate> &candidates) override;
  void Acquire() override;

  // DataChannelObserver
  void OnStateChange() override {}
  //  A data buffer was successfully received.
  void OnMessage(const webrtc::DataBuffer& buffer) override;

  // PeerConnectionObserver
  // Triggered when the SignalingState changed.
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}

  // Triggered when a remote peer opens a data channel.
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {}

  // Triggered when renegotiation is needed. For example, an ICE restart
  // has begun.
  void OnRenegotiationNeeded() override {}

  // Called any time the legacy IceConnectionState changes.
  //
  // Note that our ICE states lag behind the standard slightly. The most
  // notable differences include the fact that "failed" occurs after 15
  // seconds, not 30, and this actually represents a combination ICE + DTLS
  // state, so it may be "failed" if DTLS fails while ICE succeeds.
  //
  // TODO(jonasolsson): deprecate and remove this.
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}

  // Called any time the IceGatheringState changes.
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

  // A new ICE candidate has been gathered.
  void OnIceCandidate(
      const webrtc::IceCandidateInterface* candidate) override;

 private:
  std::string identifier_;
  SignalingMessageInterface *messaging_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
  rtc::scoped_refptr<CreateSessionDescriptionObserver> create_session_observer_;
  rtc::scoped_refptr<SetSessionDescriptionObserver> set_offer_observer_;
  rtc::scoped_refptr<SetSessionDescriptionObserver> set_answer_observer_;
  rtc::scoped_refptr<CapturerTrackSource> track_source_;
  VideoCapturer *video_capturer_;
  std::unique_ptr<RenderInstanceInterface> render_instance_;
  // gathered ICE candidates so far
  std::vector<ICECandidate> ice_candidates_;
  bool ice_buffering_;
};

}  // namespace rigel

#endif  // RIGEL_RTC_CHANNEL_H_
