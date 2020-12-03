
#ifndef RIGEL_GRAPHICS_RENDER_ENCODE_BUFFER_H_
#define RIGEL_GRAPHICS_RENDER_ENCODE_BUFFER_H_

#include <memory>

#include "api/video/encoded_image.h"
#include "rtc_base/ref_counted_object.h"

namespace rigel {

// @see https://webrtc.googlesource.com/src/+/69202b2a57b8b7f7046dc26930aafd6f779a152e/api/video/encoded_image.h
class NonCopyEncodedImageBuffer : public webrtc::EncodedImageBufferInterface {
 public:
  static rtc::scoped_refptr<NonCopyEncodedImageBuffer> Create(const uint8_t* data, size_t size);

  explicit NonCopyEncodedImageBuffer(const uint8_t *data, size_t size);
  virtual ~NonCopyEncodedImageBuffer() {}

  virtual const uint8_t* data() const override { return data_; }
  virtual uint8_t* data() override { return (uint8_t *)data_; }
  virtual size_t size() const override { return size_; }
 private:
  const uint8_t *data_;
  size_t size_;
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENCODE_BUFFER_H_
