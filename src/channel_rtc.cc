
#include "channel_rtc.h"
#include "logging.inc"
#include "capture_track_source.h"
#include "render.h"
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

namespace rigel {

RTCPeerChannel::RTCPeerChannel(const std::string &identifier,
    SignalingMessageInterface *messaging)
    : identifier_(identifier), messaging_(messaging), ice_buffering_(true),
      create_session_observer_(
          new rtc::RefCountedObject<CreateSessionDescriptionObserver>(this)),
      set_offer_observer_(
          new rtc::RefCountedObject<SetSessionDescriptionObserver>(
              this, webrtc::SdpType::kOffer)),
      set_answer_observer_(
          new rtc::RefCountedObject<SetSessionDescriptionObserver>(
              this, webrtc::SdpType::kAnswer)) {}

RTCPeerChannel::~RTCPeerChannel() {
  if (render_instance_) {
    render_instance_->StopRendering();
    render_instance_ = nullptr;
  }
}

void RTCPeerChannel::Initialize(
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> &&connection,
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> &factory,
    RenderInstanceFactoryInterface *render_instance_factory) {
  // data channel
  webrtc::DataChannelInit init;
  data_channel_ = connection->CreateDataChannel("data_channel", &init);
  data_channel_->RegisterObserver(this);
  // video capturer
  video_capturer_ = new VideoCapturer();
  // renderer
  render_instance_ = render_instance_factory->CreateInstance(video_capturer_);
  // capture source
  std::unique_ptr<
      rtc::VideoSourceInterface<webrtc::VideoFrame>> source(video_capturer_);
  track_source_ = CapturerTrackSource::Create(std::move(source));
  // create track
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
      factory->CreateVideoTrack("track0", track_source_));
  // create video sender
  rtc::scoped_refptr<webrtc::RtpSenderInterface> sender(
      connection->CreateSender(
          webrtc::MediaStreamTrackInterface::kVideoKind, "video"));
  sender->SetTrack(video_track);
  // setup complete
  connection_ = connection;
}

void RTCPeerChannel::Offer() {
  using Options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions;
  Options options;
  options.offer_to_receive_video = Options::kOfferToReceiveMediaTrue;
  connection_->CreateOffer(create_session_observer_, options);
}

void RTCPeerChannel::AcceptAnswer(const std::string &answer) {
  webrtc::SdpParseError error;
  auto *desc = webrtc::CreateSessionDescription(
      webrtc::SessionDescriptionInterface::kAnswer, answer, &error);
  if (desc == nullptr) {
    RGL_WARN("AcceptAnswer SdpParseError: " + error.description);
    return;
  }
  connection_->SetRemoteDescription(set_answer_observer_, desc);
}

void RTCPeerChannel::ReceiveICECandidates(
    const std::vector<ICECandidate> &candidates) {
  for (const auto &item : candidates) {
    webrtc::SdpParseError error;
    webrtc::IceCandidateInterface *candidate = webrtc::CreateIceCandidate(
        item.sdp_mid, item.sdp_mline_index, item.sdp, &error);
    if (candidate == nullptr) {
      RGL_WARN("ReceiveICECandidates SdpParseError: " + error.description);
      continue;
    }
    connection_->AddIceCandidate(candidate);
  }
}

void RTCPeerChannel::Acquire() {
  video_capturer_->Initialize();
  render_instance_->StartRendering();
}

void RTCPeerChannel::OnCreateSessionDescriptionSuccess(
    webrtc::SessionDescriptionInterface* desc) {
  connection_->SetLocalDescription(set_offer_observer_, desc);
  std::string sdp;
  if (!desc->ToString(&sdp)) return;
  messaging_->SendOffer(identifier_, sdp);
}

void RTCPeerChannel::OnSetSessionDescriptionSuccess(webrtc::SdpType sdp_type) {
  if (sdp_type == webrtc::SdpType::kAnswer) {
    // send gathered candidates
    std::vector<ICECandidate> candidates = ice_candidates_;
    ice_candidates_.clear();
    ice_buffering_ = false;
    messaging_->SendICECandidates(identifier_, candidates);
  }
}

void RTCPeerChannel::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  if (!candidate->ToString(&sdp)) return;
  auto item = ICECandidate {
    .sdp = std::move(sdp),
    .sdp_mid = candidate->sdp_mid(),
    .sdp_mline_index = candidate->sdp_mline_index()
  };
  ice_candidates_.push_back(item);
  if (!ice_buffering_) {
    RGL_INFO("flushing ice candidates");
    std::vector<ICECandidate> candidates = ice_candidates_;
    ice_candidates_.clear();
    messaging_->SendICECandidates(identifier_, candidates);
  }
}

void RTCPeerChannel::OnIceGatheringChange(
    webrtc::PeerConnectionInterface::IceGatheringState new_state) {}

void RTCPeerChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  std::string message(buffer.data.data<char>(), buffer.data.size());
  boost::char_separator<char> sep(",");
  boost::tokenizer<boost::char_separator<char> > tokens(message, sep);
  std::vector<std::string> v;
  std::copy(tokens.begin(), tokens.end(), std::back_inserter(v));
  if (v.size() == 0) return;
  const auto &command = v[0];
  // Move XY
  if (command == "m") {
    if (v.size() < 3) return;
    int x, y;
    try {
      x = boost::lexical_cast<int>(v[1]);
      y = boost::lexical_cast<int>(v[2]);
    } catch (boost::bad_lexical_cast const &e) {
        return;
    }
    render_instance_->InputXYAxis(x, y);
  }
  // Wheel
  if (command == "w") {
    if (v.size() < 2) return;
    int z;
    try {
      z = boost::lexical_cast<int>(v[1]);
    } catch (boost::bad_lexical_cast const &e) {
        return;
    }
    render_instance_->InputZAxis(z);
  }
}

}  // namespace rigel
