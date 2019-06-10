

#include "render.h"
#include "render_instance.h"

namespace rigel {

std::unique_ptr<RenderInstanceInterface> RenderContext::CreateInstance(
    RenderInstanceSink *sink) {
  return std::unique_ptr<RenderInstanceInterface>(new RenderInstance(sink));
}

}  // namespace rigel
