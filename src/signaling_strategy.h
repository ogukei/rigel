
#ifndef RIGEL_BASE_SIGNALING_STRATEGY_H_
#define RIGEL_BASE_SIGNALING_STRATEGY_H_

namespace rigel {

struct SignalingStrategy {
  virtual ~SignalingStrategy() = default;
  virtual void Initialize() = 0;
};

}  // namespace rigel

#endif  // RIGEL_BASE_SIGNALING_STRATEGY_H_
