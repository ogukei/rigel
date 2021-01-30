
/**
  Most code of this file ported from:
    Vulkan Example (Copyright (C) 2017 by Sascha Willems)
 */

#include <vector>
#include <array>
#include <iostream>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vulkan/vulkan.h>

#include "render.h"
#include "render_engine.h"
#include "render_context_cuda.h"
#include "render_engine_cuda.h"
#include "render_helper.inc"
#include "logging.inc"

#define TINYOBJLOADER_IMPLEMENTATION
#include "third_party/tiny_obj_loader.h"


#define VK_CHECK_RESULT(f) (f)

namespace rigel {

class GraphicsRendererImpl {
 public:
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  uint32_t queueFamilyIndex;
  VkPipelineCache pipelineCache;
  VkQueue queue;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  std::vector<VkShaderModule> shaderModules;
  VkBuffer vertexBuffer, indexBuffer;
  VkDeviceMemory vertexMemory, indexMemory;

  const char* imagedata;
  VkImage dstImage;
  VkDeviceMemory dstImageMemory;
  VkCommandBuffer copyCmd;

  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
  };
  int32_t width, height;
  VkFramebuffer framebuffer;
  FrameBufferAttachment colorAttachment, depthAttachment;
  VkRenderPass renderPass;

  uint32_t draw_index_count_;

  // CUDA interop
  RenderContextCuda *cuda_;
  RenderEngineCuda *cuda_engine_;
  VkSubresourceLayout dstImageSubResourceLayout;

  uint32_t GetMemoryTypeIndex(uint32_t typeBits,
      VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice,
        &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
      if ((typeBits & 1) == 1) {
        if ((deviceMemoryProperties.memoryTypes[i].
            propertyFlags & properties) == properties) {
          return i;
        }
      }
      typeBits >>= 1;
    }
    return 0;
  }

  VkResult CreateBuffer(VkBufferUsageFlags usageFlags,
      VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *buffer,
      VkDeviceMemory *memory, VkDeviceSize size, void *data = nullptr) {
    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo =
      CreateBufferCreateInfo(usageFlags, size);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = CreateMemoryAllocateInfo();
    vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits,
        memoryPropertyFlags);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));
    if (data != nullptr) {
      void *mapped;
      VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
      memcpy(mapped, data, size);
      vkUnmapMemory(device, *memory);
    }
    VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));
    return VK_SUCCESS;
  }

  /*
    Submit command buffer to a queue and wait for fence until queue operations have been finished
  */
  void SubmitWork(VkCommandBuffer cmdBuffer, VkQueue queue) {
    VkSubmitInfo submitInfo = CreateSubmitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    VkFenceCreateInfo fenceInfo = CreateFenceCreateInfo();
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fence, nullptr);
  }

  GraphicsRendererImpl(RenderContext *context) : cuda_(context->Cuda()) {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Rigel";
    appInfo.pEngineName = "RIGEL";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    /*
      Vulkan instance creation (without surface extensions)
    */
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    /*
      Vulkan device creation
    */
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance,
        &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance,
        &deviceCount, physicalDevices.data()));
    physicalDevice = physicalDevices[0];

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    RGL_INFO(std::string("GPU: ") + deviceProperties.deviceName);

    // Request a single graphics queue
    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
        &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(
        queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
        queueFamilyProperties.data());
    {
      uint32_t size = static_cast<uint32_t>(queueFamilyProperties.size());
      for (uint32_t i = 0; i < size; i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          queueFamilyIndex = i;
          queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
          queueCreateInfo.queueFamilyIndex = i;
          queueCreateInfo.queueCount = 1;
          queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
          break;
        }
      }
    }
    // extensions
    const std::vector<const char*> deviceExtensions = {
      VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
      VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
      VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
      VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
    };
    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice,
        &deviceCreateInfo, nullptr, &device));

    // Get a graphics queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // Command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device,
        &cmdPoolInfo, nullptr, &commandPool));

    /*
      Prepare vertex and index buffers
    */
    struct Vertex {
      float position[3];
      float color[3];
    };
    {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;
      {
        std::string inputfile = "res/cube.obj";

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes,
            &materials, &warn, &err, inputfile.c_str());
        if (!ret) {
          RGL_WARN("count not load " + inputfile);
          exit(1);
        }
        for (int i = 0; i < attrib.vertices.size(); i += 3) {
          const auto &x = attrib.vertices[i + 0];
          const auto &y = attrib.vertices[i + 1];
          const auto &z = attrib.vertices[i + 2];
          Vertex vtx = {
            { x, z, y },
            {
              (glm::cos(i * 0.324f) + 1.0f) * 0.5f,
              (glm::cos(i * 0.513f) + 1.0f) * 0.5f,
              (glm::cos(i + 0.762f) + 1.0f) * 0.5f
            }
          };
          vertices.push_back(vtx);
        }
        for (size_t s = 0; s < shapes.size(); s++) {
          size_t index_offset = 0;
          for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];
            for (size_t v = 0; v < fv; v++) {
              tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
              indices.push_back(idx.vertex_index);
            }
            index_offset += fv;
          }
        }
      }
      draw_index_count_ = indices.size();

      const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
      const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingMemory;

      // Command buffer for copy commands (reused)
      VkCommandBufferAllocateInfo cmdBufAllocateInfo =
          CreateCommandBufferAllocateInfo(commandPool,
              VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
      VkCommandBuffer copyCmd;
      VK_CHECK_RESULT(vkAllocateCommandBuffers(device,
          &cmdBufAllocateInfo, &copyCmd));
      VkCommandBufferBeginInfo cmdBufInfo = CreateCommandBufferBeginInfo();

      // Copy input data to VRAM using a staging buffer
      {
        // Vertices
        CreateBuffer(
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &stagingBuffer,
          &stagingMemory,
          vertexBufferSize,
          vertices.data());

        CreateBuffer(
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          &vertexBuffer,
          &vertexMemory,
          vertexBufferSize);

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
        VkBufferCopy copyRegion = {};
        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, stagingBuffer, vertexBuffer, 1, &copyRegion);
        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        SubmitWork(copyCmd, queue);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // Indices
        CreateBuffer(
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &stagingBuffer,
          &stagingMemory,
          indexBufferSize,
          indices.data());

        CreateBuffer(
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          &indexBuffer,
          &indexMemory,
          indexBufferSize);

        VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, stagingBuffer, indexBuffer, 1, &copyRegion);
        VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

        SubmitWork(copyCmd, queue);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
      }
    }

    /*
      Create framebuffer attachments
    */
    width = 960;
    height = 544;
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat;
    GetSupportedDepthFormat(physicalDevice, &depthFormat);
    {
      // Color attachment
      VkImageCreateInfo image = CreateImageCreateInfo();
      image.imageType = VK_IMAGE_TYPE_2D;
      image.format = colorFormat;
      image.extent.width = width;
      image.extent.height = height;
      image.extent.depth = 1;
      image.mipLevels = 1;
      image.arrayLayers = 1;
      image.samples = VK_SAMPLE_COUNT_1_BIT;
      image.tiling = VK_IMAGE_TILING_OPTIMAL;
      image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
          | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

      VkMemoryAllocateInfo memAlloc = CreateMemoryAllocateInfo();
      VkMemoryRequirements memReqs;

      VK_CHECK_RESULT(vkCreateImage(device,
          &image, nullptr, &colorAttachment.image));
      vkGetImageMemoryRequirements(device, colorAttachment.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device,
          &memAlloc, nullptr, &colorAttachment.memory));
      VK_CHECK_RESULT(vkBindImageMemory(device,
          colorAttachment.image, colorAttachment.memory, 0));

      VkImageViewCreateInfo colorImageView = CreateImageViewCreateInfo();
      colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      colorImageView.format = colorFormat;
      colorImageView.subresourceRange = {};
      colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      colorImageView.subresourceRange.baseMipLevel = 0;
      colorImageView.subresourceRange.levelCount = 1;
      colorImageView.subresourceRange.baseArrayLayer = 0;
      colorImageView.subresourceRange.layerCount = 1;
      colorImageView.image = colorAttachment.image;
      VK_CHECK_RESULT(vkCreateImageView(device,
          &colorImageView, nullptr, &colorAttachment.view));

      // Depth stencil attachment
      image.format = depthFormat;
      image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

      VK_CHECK_RESULT(vkCreateImage(device,
          &image, nullptr, &depthAttachment.image));
      vkGetImageMemoryRequirements(device, depthAttachment.image, &memReqs);
      memAlloc.allocationSize = memReqs.size;
      memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      VK_CHECK_RESULT(vkAllocateMemory(device,
          &memAlloc, nullptr, &depthAttachment.memory));
      VK_CHECK_RESULT(vkBindImageMemory(device,
          depthAttachment.image, depthAttachment.memory, 0));

      VkImageViewCreateInfo depthStencilView = CreateImageViewCreateInfo();
      depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
      depthStencilView.format = depthFormat;
      depthStencilView.flags = 0;
      depthStencilView.subresourceRange = {};
      depthStencilView.subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      depthStencilView.subresourceRange.baseMipLevel = 0;
      depthStencilView.subresourceRange.levelCount = 1;
      depthStencilView.subresourceRange.baseArrayLayer = 0;
      depthStencilView.subresourceRange.layerCount = 1;
      depthStencilView.image = depthAttachment.image;
      VK_CHECK_RESULT(vkCreateImageView(device,
          &depthStencilView, nullptr, &depthAttachment.view));
    }

    /*
      Create renderpass
    */
    {
      std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
      {
        // Color attachment
        auto &color = attchmentDescriptions[0];
        color.format = colorFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        // Depth attachment
        auto &depth = attchmentDescriptions[1];
        depth.format = depthFormat;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      }

      VkAttachmentReference colorReference =
          { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
      VkAttachmentReference depthReference =
          { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

      VkSubpassDescription subpassDescription = {};
      subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpassDescription.colorAttachmentCount = 1;
      subpassDescription.pColorAttachments = &colorReference;
      subpassDescription.pDepthStencilAttachment = &depthReference;

      // Use subpass dependencies for layout transitions
      std::array<VkSubpassDependency, 2> dependencies;

      dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[0].dstSubpass = 0;
      dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[0].dstStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      dependencies[0].dstAccessMask =
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      dependencies[1].srcSubpass = 0;
      dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
      dependencies[1].srcStageMask =
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependencies[1].dstStageMask =
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
      dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

      // Create the actual renderpass
      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount =
          static_cast<uint32_t>(attchmentDescriptions.size());
      renderPassInfo.pAttachments = attchmentDescriptions.data();
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpassDescription;
      renderPassInfo.dependencyCount =
          static_cast<uint32_t>(dependencies.size());
      renderPassInfo.pDependencies = dependencies.data();
      VK_CHECK_RESULT(vkCreateRenderPass(device,
          &renderPassInfo, nullptr, &renderPass));

      VkImageView attachments[2];
      attachments[0] = colorAttachment.view;
      attachments[1] = depthAttachment.view;

      VkFramebufferCreateInfo framebufferCreateInfo =
          CreateFramebufferCreateInfo();
      framebufferCreateInfo.renderPass = renderPass;
      framebufferCreateInfo.attachmentCount = 2;
      framebufferCreateInfo.pAttachments = attachments;
      framebufferCreateInfo.width = width;
      framebufferCreateInfo.height = height;
      framebufferCreateInfo.layers = 1;
      VK_CHECK_RESULT(vkCreateFramebuffer(device,
          &framebufferCreateInfo, nullptr, &framebuffer));
    }

    /* 
      Prepare graphics pipeline
    */
    {
      std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
      VkDescriptorSetLayoutCreateInfo descriptorLayout =
          CreateDescriptorSetLayoutCreateInfo(setLayoutBindings);
      VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device,
          &descriptorLayout, nullptr, &descriptorSetLayout));

      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
          CreatePipelineLayoutCreateInfo(nullptr, 0);

      // MVP via push constant block
      VkPushConstantRange pushConstantRange =
          CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT,
              sizeof(glm::mat4), 0);
      std::vector<VkPushConstantRange> ranges;
      ranges.push_back(pushConstantRange);
      pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
      pipelineLayoutCreateInfo.pPushConstantRanges = ranges.data();

      VK_CHECK_RESULT(vkCreatePipelineLayout(device,
          &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

      VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
      pipelineCacheCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
      VK_CHECK_RESULT(vkCreatePipelineCache(device,
          &pipelineCacheCreateInfo, nullptr, &pipelineCache));

      // Create pipeline
      VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
          CreatePipelineInputAssemblyStateCreateInfo(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

      VkPipelineRasterizationStateCreateInfo rasterizationState =
          CreatePipelineRasterizationStateCreateInfo(
              VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
              VK_FRONT_FACE_COUNTER_CLOCKWISE);

      VkPipelineColorBlendAttachmentState blendAttachmentState =
          CreatePipelineColorBlendAttachmentState(0xf, VK_FALSE);

      VkPipelineColorBlendStateCreateInfo colorBlendState =
          CreatePipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

      VkPipelineDepthStencilStateCreateInfo depthStencilState =
          CreatePipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE,
              VK_COMPARE_OP_LESS_OR_EQUAL);

      VkPipelineViewportStateCreateInfo viewportState =
          CreatePipelineViewportStateCreateInfo(1, 1);

      VkPipelineMultisampleStateCreateInfo multisampleState =
          CreatePipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

      std::vector<VkDynamicState> dynamicStateEnables = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
      };
      VkPipelineDynamicStateCreateInfo dynamicState =
        CreatePipelineDynamicStateCreateInfo(dynamicStateEnables);

      VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        CreatePipelineCreateInfo(pipelineLayout, renderPass);

      std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

      pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
      pipelineCreateInfo.pRasterizationState = &rasterizationState;
      pipelineCreateInfo.pColorBlendState = &colorBlendState;
      pipelineCreateInfo.pMultisampleState = &multisampleState;
      pipelineCreateInfo.pViewportState = &viewportState;
      pipelineCreateInfo.pDepthStencilState = &depthStencilState;
      pipelineCreateInfo.pDynamicState = &dynamicState;
      pipelineCreateInfo.stageCount =
          static_cast<uint32_t>(shaderStages.size());
      pipelineCreateInfo.pStages = shaderStages.data();

      // Vertex bindings an attributes
      // Binding description
      std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
        CreateVertexInputBindingDescription(0,
            sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
      };

      // Attribute descriptions
      std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        // position
        CreateVertexInputAttributeDescription(0, 0,
            VK_FORMAT_R32G32B32_SFLOAT, 0),
        // color
        CreateVertexInputAttributeDescription(0, 1,
            VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),
      };

      VkPipelineVertexInputStateCreateInfo vertexInputState =
          CreatePipelineVertexInputStateCreateInfo();
      vertexInputState.vertexBindingDescriptionCount =
          static_cast<uint32_t>(vertexInputBindings.size());
      vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
      vertexInputState.vertexAttributeDescriptionCount =
          static_cast<uint32_t>(vertexInputAttributes.size());
      vertexInputState.pVertexAttributeDescriptions =
          vertexInputAttributes.data();

      pipelineCreateInfo.pVertexInputState = &vertexInputState;

      shaderStages[0].sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      shaderStages[0].pName = "main";
      shaderStages[1].sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      shaderStages[1].pName = "main";
      shaderStages[0].module = LoadShader("shaders/triangle.vert.spv", device);
      shaderStages[1].module = LoadShader("shaders/triangle.frag.spv", device);

      shaderModules = { shaderStages[0].module, shaderStages[1].module };
      VK_CHECK_RESULT(vkCreateGraphicsPipelines(device,
          pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
    }

    /* 
      Command buffer creation
    */
    PrepareCapture();
    PrepareCaptureTwo();
    PrepareCaptureThree();
  }

  void PrepareCapture() {
    /*
      Copy framebuffer image to host visible image
    */
    // Create the linear tiled destination image
    // to copy to and to read the memory from
    VkImageCreateInfo imgCreateInfo = CreateImageCreateInfo();
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imgCreateInfo.extent.width = width;
    imgCreateInfo.extent.height = height;
    imgCreateInfo.extent.depth = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // specify external
    VkExternalMemoryImageCreateInfo vkExternalMemImageCreateInfo = {};
    vkExternalMemImageCreateInfo.sType =
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    vkExternalMemImageCreateInfo.pNext = NULL;
    vkExternalMemImageCreateInfo.handleTypes =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    imgCreateInfo.pNext = &vkExternalMemImageCreateInfo;

    // Create the image
    VK_CHECK_RESULT(vkCreateImage(device, &imgCreateInfo, nullptr, &dstImage));
    // Create memory to back up the image
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo = CreateMemoryAllocateInfo();
    vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    // Memory must be host visible to copy from
    memAllocInfo.memoryTypeIndex =
        GetMemoryTypeIndex(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // specify external
    VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
    vulkanExportMemoryAllocateInfoKHR.sType =
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
    vulkanExportMemoryAllocateInfoKHR.pNext = NULL;
    vulkanExportMemoryAllocateInfoKHR.handleTypes =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    memAllocInfo.pNext = &vulkanExportMemoryAllocateInfoKHR;
    // alloc
    VK_CHECK_RESULT(vkAllocateMemory(device,
        &memAllocInfo, nullptr, &dstImageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, dstImage, dstImageMemory, 0));
  }

  void PrepareCaptureTwo() {
    // Do the actual blit from the offscreen image to
    // our host visible destination image
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        CreateCommandBufferAllocateInfo(commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device,
        &cmdBufAllocateInfo, &copyCmd));
    VkCommandBufferBeginInfo cmdBufInfo = CreateCommandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    // Transition destination image to transfer destination layout
    InsertImageMemoryBarrier(
      copyCmd,
      dstImage,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    // colorAttachment.image is already in
    // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    // and does not need to be transitioned
    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = width;
    imageCopyRegion.extent.height = height;
    imageCopyRegion.extent.depth = 1;

    vkCmdCopyImage(
      copyCmd,
      colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageCopyRegion);

    // Transition destination image to general layout,
    // which is the required layout for mapping the image memory later on
    InsertImageMemoryBarrier(
      copyCmd,
      dstImage,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    SubmitWork(copyCmd, queue);
    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkGetImageSubresourceLayout(device,
        dstImage, &subResource, &dstImageSubResourceLayout);
  }

  void PrepareCaptureThree() {
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo = CreateMemoryAllocateInfo();
    vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
    cuda_engine_ = new RenderEngineCuda(instance, device, dstImageMemory, memRequirements.size);
  }

  void Render(float phi, float theta, float gamma) {
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        CreateCommandBufferAllocateInfo(commandPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device,
        &cmdBufAllocateInfo, &commandBuffer));

    VkCommandBufferBeginInfo cmdBufInfo = CreateCommandBufferBeginInfo();

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;

    vkCmdBeginRenderPass(commandBuffer,
        &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.height = static_cast<float>(height);
    viewport.width = static_cast<float>(width);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Render scene
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    std::vector<glm::vec3> pos = {
      glm::vec3(0, 0, 0)
    };

    for (const auto &v : pos) {
      glm::mat4 proj(glm::perspective(
          glm::radians(60.0f),
          static_cast<float>(width) / static_cast<float>(height),
          0.1f, 256.0f));
      theta -= 1.72f;
      float dist = 15.0f;
      dist += gamma;
      float x = dist * glm::sin(theta) * glm::cos(phi + 1.0);
      float y = dist * glm::cos(theta);
      float z = dist * glm::sin(theta) * glm::sin(phi + 1.0);
      glm::vec3 eye(x, y, z);
      auto view = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
      auto model = glm::translate(glm::mat4(), v);
      glm::mat4 mvp = proj * view;
      vkCmdPushConstants(commandBuffer, pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);
      vkCmdDrawIndexed(commandBuffer, draw_index_count_, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    SubmitWork(commandBuffer, queue);
  }

  void Capture(const RGLGraphicsCaptureHandle &handle) {
    SubmitWork(copyCmd, queue);
    handle((const char *)(uintptr_t)cuda_engine_->DevicePointer(), width, height, (int)dstImageSubResourceLayout.rowPitch);
  }

  ~GraphicsRendererImpl() {
    // Clean up resources
    delete cuda_engine_;
    cuda_engine_ = nullptr;

    vkFreeMemory(device, dstImageMemory, nullptr);
    vkDestroyImage(device, dstImage, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexMemory, nullptr);
    vkDestroyImageView(device, colorAttachment.view, nullptr);
    vkDestroyImage(device, colorAttachment.image, nullptr);
    vkFreeMemory(device, colorAttachment.memory, nullptr);
    vkDestroyImageView(device, depthAttachment.view, nullptr);
    vkDestroyImage(device, depthAttachment.image, nullptr);
    vkFreeMemory(device, depthAttachment.memory, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    for (auto shadermodule : shaderModules) {
      vkDestroyShaderModule(device, shadermodule, nullptr);
    }
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
  }
};

GraphicsRenderer::GraphicsRenderer(RenderContext *context) : impl_(new GraphicsRendererImpl(context)) {}

GraphicsRenderer::~GraphicsRenderer() {
  delete impl_;
}

void GraphicsRenderer::Render(float x, float y, float z) {
  impl_->Render(x, y, z);
}

void GraphicsRenderer::Capture(const RGLGraphicsCaptureHandle &f) {
  impl_->Capture(f);
}

}  // namespace rigel
