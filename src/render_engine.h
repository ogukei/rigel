
#ifndef RIGEL_GRAPHICS_RENDER_ENGINE_H_
#define RIGEL_GRAPHICS_RENDER_ENGINE_H_

#include <functional>

namespace rigel {

class GraphicsRendererImpl;

typedef std::function<void(const char *, int, int, int)>
    RGLGraphicsCaptureHandle;

class GraphicsRenderer {
 private:
  GraphicsRendererImpl *impl_;
 public:
  GraphicsRenderer();
  ~GraphicsRenderer();
  void Render(float x, float y, float z);
  void Capture(const RGLGraphicsCaptureHandle &f);
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENGINE_H_
