
#ifndef RIGEL_GRAPHICS_RENDER_ENGINE_CUDA_H_
#define RIGEL_GRAPHICS_RENDER_ENGINE_CUDA_H_

#include <cuda.h>
#include <vulkan/vulkan.h>

namespace rigel {

class RenderEngineCudaPrivate;

class RenderEngineCuda {
 public:
  RenderEngineCuda(
    VkInstance instance, 
    VkDevice device, 
    VkDeviceMemory memory,
    VkDeviceSize memory_size);
  ~RenderEngineCuda();

  CUdeviceptr DevicePointer() const {
    return device_ptr_;
  }
 private:
  RenderEngineCudaPrivate *private_;
  CUdeviceptr device_ptr_;
};

}  // namespace rigel

#endif  // RIGEL_GRAPHICS_RENDER_ENGINE_CUDA_H_
