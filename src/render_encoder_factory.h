
#ifndef RIGEL_GRAPHICS_RENDER_ENCODER_FACTORY_H_
#define RIGEL_GRAPHICS_RENDER_ENCODER_FACTORY_H_

#include <functional>
#include <memory>

#include "api/video_codecs/video_encoder_factory.h"

namespace rigel {

std::unique_ptr<webrtc::VideoEncoderFactory> CreateVideoEncoderFactory();

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENCODER_FACTORY_H_
