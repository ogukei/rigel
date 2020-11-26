
/**
  Most code of this file ported from:
    Vulkan Example (Copyright (C) 2017 by Sascha Willems)
 */

#include <vector>
#include <fstream>
#include <vulkan/vulkan.h>

namespace {
  VkBufferCreateInfo CreateBufferCreateInfo(
      VkBufferUsageFlags usage, VkDeviceSize size) {
    VkBufferCreateInfo bufCreateInfo {};
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCreateInfo.usage = usage;
    bufCreateInfo.size = size;
    return bufCreateInfo;
  }

  VkSubmitInfo CreateSubmitInfo() {
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    return submitInfo;
  }

  VkFenceCreateInfo CreateFenceCreateInfo(VkFenceCreateFlags flags = 0) {
    VkFenceCreateInfo fenceCreateInfo {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = flags;
    return fenceCreateInfo;
  }

  VkMemoryAllocateInfo CreateMemoryAllocateInfo() {
    VkMemoryAllocateInfo memAllocInfo {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    return memAllocInfo;
  }

  VkCommandBufferAllocateInfo CreateCommandBufferAllocateInfo(
      VkCommandPool commandPool,
      VkCommandBufferLevel level, uint32_t bufferCount) {
    VkCommandBufferAllocateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = commandPool;
    info.level = level;
    info.commandBufferCount = bufferCount;
    return info;
  }

  VkCommandBufferBeginInfo CreateCommandBufferBeginInfo() {
    VkCommandBufferBeginInfo cmdBufferBeginInfo {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    return cmdBufferBeginInfo;
  }

  VkImageCreateInfo CreateImageCreateInfo() {
    VkImageCreateInfo imageCreateInfo {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    return imageCreateInfo;
  }

  VkImageViewCreateInfo CreateImageViewCreateInfo() {
    VkImageViewCreateInfo imageViewCreateInfo {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    return imageViewCreateInfo;
  }

  VkFramebufferCreateInfo CreateFramebufferCreateInfo() {
    VkFramebufferCreateInfo framebufferCreateInfo {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    return framebufferCreateInfo;
  }

  VkDescriptorSetLayoutCreateInfo CreateDescriptorSetLayoutCreateInfo(
      const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    return info;
  }

  VkPipelineLayoutCreateInfo CreatePipelineLayoutCreateInfo(
      const VkDescriptorSetLayout* pSetLayouts,
      uint32_t setLayoutCount = 1) {
    VkPipelineLayoutCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = setLayoutCount;
    info.pSetLayouts = pSetLayouts;
    return info;
  }

  VkPushConstantRange CreatePushConstantRange(
      VkShaderStageFlags stageFlags,
      uint32_t size, uint32_t offset) {
    VkPushConstantRange pushConstantRange {};
    pushConstantRange.stageFlags = stageFlags;
    pushConstantRange.offset = offset;
    pushConstantRange.size = size;
    return pushConstantRange;
  }

  VkPipelineInputAssemblyStateCreateInfo
    CreatePipelineInputAssemblyStateCreateInfo(
      VkPrimitiveTopology topology,
      VkPipelineInputAssemblyStateCreateFlags flags,
      VkBool32 primitiveRestartEnable) {
    VkPipelineInputAssemblyStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = topology;
    info.flags = flags;
    info.primitiveRestartEnable = primitiveRestartEnable;
    return info;
  }

  VkPipelineRasterizationStateCreateInfo
    CreatePipelineRasterizationStateCreateInfo(
      VkPolygonMode polygonMode,
      VkCullModeFlags cullMode,
      VkFrontFace frontFace,
      VkPipelineRasterizationStateCreateFlags flags = 0) {
    VkPipelineRasterizationStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.polygonMode = polygonMode;
    info.cullMode = cullMode;
    info.frontFace = frontFace;
    info.flags = flags;
    info.depthClampEnable = VK_FALSE;
    info.lineWidth = 1.0f;
    return info;
  }

  VkPipelineColorBlendAttachmentState
    CreatePipelineColorBlendAttachmentState(
      VkColorComponentFlags colorWriteMask,
      VkBool32 blendEnable) {
    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
    pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
    pipelineColorBlendAttachmentState.blendEnable = blendEnable;
    return pipelineColorBlendAttachmentState;
  }

  VkPipelineColorBlendStateCreateInfo
    CreatePipelineColorBlendStateCreateInfo(
      uint32_t attachmentCount,
      const VkPipelineColorBlendAttachmentState * pAttachments) {
    VkPipelineColorBlendStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.attachmentCount = attachmentCount;
    info.pAttachments = pAttachments;
    return info;
  }

  VkPipelineDepthStencilStateCreateInfo
    CreatePipelineDepthStencilStateCreateInfo(
      VkBool32 depthTestEnable,
      VkBool32 depthWriteEnable,
      VkCompareOp depthCompareOp) {
    VkPipelineDepthStencilStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable = depthTestEnable;
    info.depthWriteEnable = depthWriteEnable;
    info.depthCompareOp = depthCompareOp;
    info.front = info.back;
    info.back.compareOp = VK_COMPARE_OP_ALWAYS;
    return info;
  }

  VkPipelineViewportStateCreateInfo
    CreatePipelineViewportStateCreateInfo(
      uint32_t viewportCount, uint32_t scissorCount,
      VkPipelineViewportStateCreateFlags flags = 0) {
    VkPipelineViewportStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = viewportCount;
    info.scissorCount = scissorCount;
    info.flags = flags;
    return info;
  }

  VkPipelineMultisampleStateCreateInfo
    CreatePipelineMultisampleStateCreateInfo(
      VkSampleCountFlagBits rasterizationSamples,
      VkPipelineMultisampleStateCreateFlags flags = 0) {
    VkPipelineMultisampleStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.rasterizationSamples = rasterizationSamples;
    info.flags = flags;
    return info;
  }

  VkPipelineDynamicStateCreateInfo CreatePipelineDynamicStateCreateInfo(
      const std::vector<VkDynamicState>& pDynamicStates,
      VkPipelineDynamicStateCreateFlags flags = 0) {
    VkPipelineDynamicStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.pDynamicStates = pDynamicStates.data();
    info.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
    info.flags = flags;
    return info;
  }

  VkGraphicsPipelineCreateInfo CreatePipelineCreateInfo(
      VkPipelineLayout layout, VkRenderPass renderPass,
      VkPipelineCreateFlags flags = 0) {
    VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.flags = flags;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    return pipelineCreateInfo;
  }

  VkVertexInputBindingDescription CreateVertexInputBindingDescription(
      uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
    VkVertexInputBindingDescription vInputBindDescription {};
    vInputBindDescription.binding = binding;
    vInputBindDescription.stride = stride;
    vInputBindDescription.inputRate = inputRate;
    return vInputBindDescription;
  }

  VkVertexInputAttributeDescription CreateVertexInputAttributeDescription(
      uint32_t binding,
      uint32_t location,
      VkFormat format,
      uint32_t offset) {
    VkVertexInputAttributeDescription vInputAttribDescription {};
    vInputAttribDescription.location = location;
    vInputAttribDescription.binding = binding;
    vInputAttribDescription.format = format;
    vInputAttribDescription.offset = offset;
    return vInputAttribDescription;
  }

  VkPipelineVertexInputStateCreateInfo
      CreatePipelineVertexInputStateCreateInfo() {
    VkPipelineVertexInputStateCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    return info;
  }

  VkImageMemoryBarrier CreateImageMemoryBarrier() {
    VkImageMemoryBarrier imageMemoryBarrier {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    return imageMemoryBarrier;
  }

  void InsertImageMemoryBarrier(
      VkCommandBuffer cmdbuffer,
      VkImage image,
      VkAccessFlags srcAccessMask,
      VkAccessFlags dstAccessMask,
      VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout,
      VkPipelineStageFlags srcStageMask,
      VkPipelineStageFlags dstStageMask,
      VkImageSubresourceRange subresourceRange) {
    VkImageMemoryBarrier imageMemoryBarrier = CreateImageMemoryBarrier();
    imageMemoryBarrier.srcAccessMask = srcAccessMask;
    imageMemoryBarrier.dstAccessMask = dstAccessMask;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    vkCmdPipelineBarrier(
      cmdbuffer,
      srcStageMask,
      dstStageMask,
      0,
      0, nullptr,
      0, nullptr,
      1, &imageMemoryBarrier);
  }

  VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice,
      VkFormat *depthFormat) {
    // Since all depth formats may be optional,
    // we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats) {
      VkFormatProperties formatProps;
      vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
      // Format must support depth stencil attachment for optimal tiling
      if (formatProps.optimalTilingFeatures &
          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        *depthFormat = format;
        return true;
      }
    }

    return false;
  }

  VkShaderModule LoadShader(const char *fileName, VkDevice device) {
    std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);
    if (is.is_open()) {
      size_t size = is.tellg();
      is.seekg(0, std::ios::beg);
      char* shaderCode = new char[size];
      is.read(shaderCode, size);
      is.close();

      VkShaderModule shaderModule;
      VkShaderModuleCreateInfo moduleCreateInfo{};
      moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      moduleCreateInfo.codeSize = size;
      moduleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode);

      vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);
      delete[] shaderCode;

      return shaderModule;
    } else {
      return VK_NULL_HANDLE;
    }
  }

  int GetVkImageMemoryHandleFD(VkInstance instance, VkDevice device, VkDeviceMemory imageMemory) {
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
}  // unnamed namespace
