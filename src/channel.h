
#ifndef RIGEL_BASE_CHANNEL_H_
#define RIGEL_BASE_CHANNEL_H_

#include <string>
#include <vector>

#include "ice_candidate.h"

namespace rigel {

struct PeerChannelInterface {
  virtual ~PeerChannelInterface() = default;
  virtual void Offer() = 0;
  virtual void AcceptAnswer(const std::string &answer) = 0;
  virtual void Acquire() = 0;
  virtual void ReceiveICECandidates(
      const std::vector<ICECandidate> &candidates) = 0;
};

}  // namespace rigel

#endif  // RIGEL_BASE_CHANNEL_H_
