
#include "capture_frame.h"
#include <stdexcept>

namespace rigel {

NativeVideoFrameBuffer::NativeVideoFrameBuffer(int width, int height)
    : width_(width), height_(height)
{
  
}

rtc::scoped_refptr<webrtc::I420BufferInterface> NativeVideoFrameBuffer::ToI420() {
  throw std::runtime_error("NativeVideoFrameBuffer::ToI420() I420 format not supported");
}

}  // namespace rigel

