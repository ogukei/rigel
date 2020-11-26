

#include "render.h"
#include "render_instance.h"
#include "render_context_cuda.h"

namespace rigel {

RenderContext::RenderContext() : cuda_(new RenderContextCuda()) {

}

RenderContext::~RenderContext() {
  delete cuda_;
  cuda_ = nullptr;
}

std::unique_ptr<RenderInstanceInterface> RenderContext::CreateInstance(
    RenderInstanceSink *sink) {
  return std::unique_ptr<RenderInstanceInterface>(new RenderInstance(sink, this));
}

}  // namespace rigel
