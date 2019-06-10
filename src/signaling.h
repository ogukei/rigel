
#ifndef RIGEL_BASE_SIGNALING_H_
#define RIGEL_BASE_SIGNALING_H_

#include <string>
#include <memory>

#include "signaling_strategy.h"
#include "signaling_sink.h"

namespace rigel {

class SignalingContext : public SignalingInstanceFactoryInterface {
 public:
  SignalingContext(const std::string &hostname,
      const std::string &port, const std::string &path);
  explicit SignalingContext(const SignalingContext &) = delete;
  ~SignalingContext();

  void Run();

  std::unique_ptr<SignalingInstanceInterface> CreateInstance(
      std::unique_ptr<SignalingControlInterface> control) override;

 private:
  std::unique_ptr<SignalingStrategy> strategy_;
};

}  // namespace rigel

#endif  // RIGEL_BASE_SIGNALING_H_
