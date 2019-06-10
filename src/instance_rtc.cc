
#include "instance_rtc.h"
#include "channel_rtc.h"
#include "logging.inc"

#include "api/create_peerconnection_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"

static webrtc::PeerConnectionInterface::RTCConfiguration MakeConfiguration() {
  webrtc::PeerConnectionInterface::RTCConfiguration configuration;
  webrtc::PeerConnectionInterface::IceServer ice_server;
  ice_server.uri = "stun:stun.l.google.com:19302";
  configuration.servers.push_back(ice_server);
  return configuration;
}

namespace rigel {

RTCInstance::RTCInstance() {
  RGL_INFO("Creating RTCInstance");
  // Network Thread
  network_thread_ = rtc::Thread::CreateWithSocketServer();
  network_thread_->Start();
  // Worker Thread
  worker_thread_ = rtc::Thread::Create();
  worker_thread_->Start();
  // Signaling Thread
  signaling_thread_ = rtc::Thread::Create();
  signaling_thread_->Start();
  // Peer Connection
  factory_ = webrtc::CreatePeerConnectionFactory(
    network_thread_.get(),
    worker_thread_.get(),
    signaling_thread_.get(),
    nullptr,
    webrtc::CreateBuiltinAudioEncoderFactory(),
    webrtc::CreateBuiltinAudioDecoderFactory(),
    webrtc::CreateBuiltinVideoEncoderFactory(),
    webrtc::CreateBuiltinVideoDecoderFactory(),
    nullptr,
    nullptr);
}

RTCInstance::~RTCInstance() {
  factory_ = nullptr;
  network_thread_->Stop();
  worker_thread_->Stop();
  signaling_thread_->Stop();
}

std::unique_ptr<PeerChannelInterface> RTCInstance::CreateChannel(
    const std::string &identifier,
    SignalingMessageInterface *messaging,
    RenderInstanceFactoryInterface *render_instance_factory) {
  auto *channel = new RTCPeerChannel(identifier, messaging);
  auto configuration = MakeConfiguration();
  auto connection = factory_->CreatePeerConnection(
      configuration, nullptr, nullptr, channel);
  channel->Initialize(std::move(connection), factory_, render_instance_factory);
  return std::unique_ptr<PeerChannelInterface>(std::move(channel));
}

}  // namespace rigel
