
#include "message_signaling.h"
#include "logging.inc"

#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/join.hpp>

namespace rigel {

namespace {
struct Message {
  std::string method;
  std::string parameter;
  std::string source;
  std::string destination;

  static boost::optional<Message> Decode(const std::string &message) {
    boost::property_tree::ptree tree;
    try {
      std::stringstream stream(message);
      boost::property_tree::json_parser::read_json(stream, tree);
    } catch (const boost::property_tree::json_parser_error &error) {
      return boost::none;
    }
    const auto method = tree.get_optional<std::string>("method")
        .value_or(std::string());
    const auto source = tree.get_optional<std::string>("source")
        .value_or(std::string());
    const auto parameter = tree.get_optional<std::string>("parameter")
        .value_or(std::string());
    return Message {
      .method = std::move(method),
      .parameter = std::move(parameter),
      .source = std::move(source)
    };
  }

  boost::optional<std::string> Encode() {
    boost::property_tree::ptree tree;
    tree.put("method", method);
    tree.put("parameter", parameter);
    tree.put("source", source);
    tree.put("destination", destination);
    std::stringstream stream;
    try {
      boost::property_tree::json_parser::write_json(stream, tree);
    } catch (const boost::property_tree::json_parser_error &error) {
      return boost::none;
    }
    return stream.str();
  }
};
}  // unnamed namespace

SignalingMessageDispatcher::SignalingMessageDispatcher(
    SignalingMessageIncomingSink *incoming,
    SignalingMessageOutgoingSink *outgoing)
    : incoming_(incoming), outgoing_(outgoing) {
  // OnStart
  storage_.Register("start",
      [](SignalingMessageIncomingSink *sink,
          const std::string &source, const std::string &parameter) {
    sink->OnStart(source);
    return true;
  });
  // OnClose
  storage_.Register("close",
      [](SignalingMessageIncomingSink *sink,
          const std::string &source, const std::string &parameter) {
    sink->OnClose(source);
    return true;
  });
  // OnAcceptAnswer
  storage_.Register("answer",
      [](SignalingMessageIncomingSink *sink,
          const std::string &source, const std::string &parameter) {
    sink->OnAcceptAnswer(source, parameter);
    return true;
  });
  // OnICECandidates
  storage_.Register("candidate",
      [](SignalingMessageIncomingSink *sink,
          const std::string &source, const std::string &parameter) {
    boost::property_tree::ptree tree;
    try {
      std::stringstream stream(parameter);
      boost::property_tree::json_parser::read_json(stream, tree);
    } catch (const boost::property_tree::json_parser_error &error) {
      RGL_WARN("OnICECandidates json parse failure");
      return false;
    }
    std::vector<ICECandidate> candidates;
    for (const auto &v : tree) {
      const auto child_opt = v.second.get_child_optional("");
      if (!child_opt) {
        RGL_WARN("OnICECandidates child node parse failure");
        return false;
      }
      const auto child = child_opt.get();
      const auto sdp = child.get_optional<std::string>("candidate")
          .value_or(std::string());
      const auto mid = child.get_optional<std::string>("sdpMid")
          .value_or(std::string());
      const auto index_opt = child.get_optional<int>("sdpMLineIndex");
      if (!index_opt) {
        RGL_WARN("OnICECandidates sdpMLineIndex parse failure");
        return false;
      }
      const auto index = index_opt.get();
      auto candidate = ICECandidate {
        .sdp = std::move(sdp),
        .sdp_mid = std::move(mid),
        .sdp_mline_index = index
      };
      candidates.push_back(std::move(candidate));
    }
    sink->OnICECandidates(source, candidates);
    return true;
  });
  // OnAcquire
  storage_.Register("acquire",
      [](SignalingMessageIncomingSink *sink,
          const std::string &source, const std::string &parameter) {
    sink->OnAcquire(source);
    return true;
  });
}

void SignalingMessageDispatcher::DispatchMessage(
    const std::string &message_str) {
  auto message_opt = Message::Decode(message_str);
  if (!message_opt) {
    RGL_WARN("Message::Decode failed");
    return;
  }
  const auto &message = message_opt.value();
  bool ok = storage_.TryInvoke(incoming_,
      message.method, message.source, message.parameter);
  if (!ok) {
    RGL_WARN("RMIStorage::TryInvoke failed");
  }
}

void SignalingMessageDispatcher::SendOffer(const std::string &destination,
      const std::string &sdp) {
  auto message = Message {
    .method = "offer",
    .parameter = sdp,
    .source = "",
    .destination = destination,
  };
  auto encoded_opt = message.Encode();
  if (!encoded_opt) {
    RGL_WARN("Message::Encode failed");
    return;
  }
  outgoing_->SendMessage(encoded_opt.value());
}

void SignalingMessageDispatcher::SendICECandidates(
    const std::string &destination,
    const std::vector<ICECandidate> &candidates) {
  // HAX: boost property tree cannot be used
  // when encoding json array as root node.
  std::vector<std::string> items;
  for (const auto &v : candidates) {
    boost::property_tree::ptree item;
    item.put("candidate", v.sdp);
    item.put("sdpMid", v.sdp_mid);
    item.put("sdpMLineIndex", v.sdp_mline_index);
    std::stringstream stream;
    try {
      boost::property_tree::json_parser::write_json(stream, item, false);
    } catch (const boost::property_tree::json_parser_error &error) {
      return;
    }
    std::string item_encoded = stream.str();
    items.push_back(item_encoded);
  }
  std::string array_encoded = "[" + boost::algorithm::join(items, ",") + "]";
  auto message = Message {
    .method = "candidate",
    .parameter = array_encoded,
    .source = "",
    .destination = destination,
  };
  auto encoded_opt = message.Encode();
  if (!encoded_opt) {
    RGL_WARN("Message::Encode failed");
    return;
  }
  outgoing_->SendMessage(encoded_opt.value());
}

}  // namespace rigel
