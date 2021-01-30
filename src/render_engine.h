
#ifndef RIGEL_GRAPHICS_RENDER_ENGINE_H_
#define RIGEL_GRAPHICS_RENDER_ENGINE_H_

#include <functional>

namespace rigel {

class GraphicsRendererImpl;
class RenderContext;

typedef std::function<void(const char *, int, int, int)>
    RGLGraphicsCaptureHandle;

class GraphicsRenderer {
 private:
  GraphicsRendererImpl *impl_;
 public:
  explicit GraphicsRenderer(RenderContext *context);
  ~GraphicsRenderer();
  void Render(float x, float y, float z);
  void Capture(const RGLGraphicsCaptureHandle &f);
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENGINE_H_
