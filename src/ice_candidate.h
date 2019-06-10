
#ifndef RIGEL_BASE_ICE_CANDIDATE_H_
#define RIGEL_BASE_ICE_CANDIDATE_H_

#include <string>

namespace rigel {

struct ICECandidate {
  std::string sdp;
  std::string sdp_mid;
  int sdp_mline_index;
};

}  // namespace rigel

#endif  // RIGEL_BASE_ICE_CANDIDATE_H_
