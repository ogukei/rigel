
#ifndef RIGEL_RTC_INSTANCE_H_
#define RIGEL_RTC_INSTANCE_H_

#include "observer_rtc.h"
#include "channel.h"

#include "rtc_base/thread.h"
#include "pc/video_track_source.h"
#include "api/scoped_refptr.h"
#include "api/peer_connection_interface.h"

namespace rigel {

struct SignalingMessageInterface;
struct RenderInstanceFactoryInterface;

class RTCInstance {
 public:
  RTCInstance();
  ~RTCInstance();

  std::unique_ptr<PeerChannelInterface> CreateChannel(
      const std::string &identifier,
      SignalingMessageInterface *messaging,
      RenderInstanceFactoryInterface *render_instance_factory);

 private:
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
  rtc::scoped_refptr<webrtc::VideoTrackSource> video_source_;
  std::unique_ptr<rtc::Thread> network_thread_;
  std::unique_ptr<rtc::Thread> worker_thread_;
  std::unique_ptr<rtc::Thread> signaling_thread_;
};

}  // namespace rigel

#endif  // RIGEL_RTC_INSTANCE_H_
