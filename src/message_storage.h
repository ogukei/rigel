
#ifndef RIGEL_BASE_MESSAGE_STORAGE_H_
#define RIGEL_BASE_MESSAGE_STORAGE_H_

#include <string>
#include <unordered_map>
#include <functional>

namespace rigel {

template<typename SinkType, typename... Args>
class RMIStorage {
 public:
  typedef std::function<bool(SinkType *, Args...)> SinkFunc;

  void Register(const std::string &name, SinkFunc sink) {
    sink_map_[name] = sink;
  }

  bool TryInvoke(SinkType *sink, const std::string &name, Args... args) {
    auto it = sink_map_.find(name);
    if (it == sink_map_.end()) return false;
    return it->second(sink, args...);
  }

 private:
  std::unordered_map<std::string, SinkFunc> sink_map_;
};

}  // namespace rigel

#endif  // RIGEL_BASE_MESSAGE_STORAGE_H_
