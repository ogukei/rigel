
#include "instance.h"
#include "instance_rtc.h"
#include "logging.inc"

namespace rigel {

SignalingInstance::SignalingInstance(
    std::unique_ptr<SignalingControlInterface> control)
    : control_(std::move(control)),
      message_dispatcher_(new SignalingMessageDispatcher(this, this)),
      render_context_(new RenderContext()) {}

void SignalingInstance::Initialize() {
  rtc_ = new RTCInstance();
}

SignalingInstance::~SignalingInstance() {
  // destruct all channels in advance
  channel_map_.clear();
  // threads are ready to be released
  delete rtc_;
  rtc_ = nullptr;
}

void SignalingInstance::ReceiveMessage(const std::string &message) {
  message_dispatcher_->DispatchMessage(message);
}

// SignalingMessageIncomingSink
void SignalingInstance::OnStart(const std::string &source) {
  auto channel = rtc_->CreateChannel(source,
      message_dispatcher_.get(), render_context_.get());
  channel->Offer();
  channel_map_[source] = std::move(channel);
}

void SignalingInstance::OnClose(const std::string &source) {
  const auto it = channel_map_.find(source);
  if (it == channel_map_.end()) {
    RGL_INFO("channel not found: " + source);
    return;
  }
  channel_map_.erase(it);
}

void SignalingInstance::OnAcceptAnswer(const std::string &source,
      const std::string &sdp) {
  const auto it = channel_map_.find(source);
  if (it == channel_map_.end()) {
    RGL_INFO("channel not found: " + source);
    return;
  }
  it->second->AcceptAnswer(sdp);
}

void SignalingInstance::OnICECandidates(const std::string &source,
    const std::vector<ICECandidate> &candidates) {
  const auto it = channel_map_.find(source);
  if (it == channel_map_.end()) {
    RGL_INFO("channel not found: " + source);
    return;
  }
  it->second->ReceiveICECandidates(candidates);
}

void SignalingInstance::OnAcquire(const std::string &source) {
  const auto it = channel_map_.find(source);
  if (it == channel_map_.end()) {
    RGL_INFO("channel not found: " + source);
    return;
  }
  it->second->Acquire();
}

// SignalingMessageOutgoingSink
void SignalingInstance::SendMessage(const std::string &message) {
  control_->SendMessage(message);
}


}  // namespace rigel
