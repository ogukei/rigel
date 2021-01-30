

#include "render_engine_cuda.h"

#include <cuda_runtime.h>
#include <stdexcept>

namespace rigel {

template <typename T>
static void check(T v) {
  if (v) {
    throw std::runtime_error("RenderEngineCuda runtime error");
  }
}

class RenderEngineCudaPrivate {
 public:
  cudaExternalMemory_t memory = nullptr;
};

static int GetVkImageMemoryHandleFD(VkInstance instance, VkDevice device, VkDeviceMemory imageMemory) {
  int fd;
  VkMemoryGetFdInfoKHR vkMemoryGetFdInfoKHR = {};
  vkMemoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
  vkMemoryGetFdInfoKHR.pNext = NULL;
  vkMemoryGetFdInfoKHR.memory = imageMemory;
  vkMemoryGetFdInfoKHR.handleType =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
  PFN_vkGetMemoryFdKHR fpGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetInstanceProcAddr(
      instance, "vkGetMemoryFdKHR");
  fpGetMemoryFdKHR(device, &vkMemoryGetFdInfoKHR, &fd);
  return fd;
}

RenderEngineCuda::RenderEngineCuda(
    VkInstance instance, 
    VkDevice device, 
    VkDeviceMemory memory,
    VkDeviceSize memory_size) 
: private_(new RenderEngineCudaPrivate())
{
  cudaExternalMemoryHandleDesc handle_desc = {};
  {
    handle_desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
    handle_desc.handle.fd = GetVkImageMemoryHandleFD(instance, device, memory);
    handle_desc.size = memory_size;
  }
  check(cudaImportExternalMemory(&private_->memory, &handle_desc));
  {
    cudaExternalMemoryBufferDesc buffer_desc = {};
    buffer_desc.offset = 0;
    buffer_desc.size = memory_size;
    buffer_desc.flags = 0;
    check(cudaExternalMemoryGetMappedBuffer((void **)&device_ptr_, 
        private_->memory, &buffer_desc));
  }
}

RenderEngineCuda::~RenderEngineCuda() {
  cudaDestroyExternalMemory(private_->memory);
}

}  // namespace rigel
