
#include "render_encode_buffer.h"
#include "rtc_base/ref_counted_object.h"

namespace rigel {

NonCopyEncodedImageBuffer::NonCopyEncodedImageBuffer(const uint8_t *data, size_t size) 
  : data_(data), size_(size) {

}

rtc::scoped_refptr<NonCopyEncodedImageBuffer> NonCopyEncodedImageBuffer::Create(
    const uint8_t* data,
    size_t size) {
  return new rtc::RefCountedObject<NonCopyEncodedImageBuffer>(data, size);
}

}  // namespace rigel
