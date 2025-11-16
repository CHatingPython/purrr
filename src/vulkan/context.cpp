#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/window.hpp"
#include "purrr/vulkan/buffer.hpp"
#include "purrr/vulkan/program.hpp"
#include "purrr/vulkan/sampler.hpp"
#include "purrr/vulkan/image.hpp"
#include "purrr/vulkan/renderTarget.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <array>
#include <unordered_set>

#undef max

namespace purrr::vulkan {

VkIndexType vkIndexType(IndexType type) {
  switch (type) {
  case IndexType::U8: return VK_INDEX_TYPE_UINT8;
  case IndexType::U16: return VK_INDEX_TYPE_UINT16;
  case IndexType::U32: return VK_INDEX_TYPE_UINT32;
  }

  throw std::runtime_error("Unreachable");
}

Context::Context(const ContextInfo &info)
  : purrr::platform::Context(info) {
  auto deviceExtensions = std::vector<const char *>({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

  createInstance(info);
  chooseDevice(deviceExtensions);
  createDevice(deviceExtensions);
  getQueue();
  createCommandPool();
  allocateCommandBuffer();
  createFence();
  createDescriptorSetLayouts();
  createDescriptorPool();
}

Context::~Context() {
  if (mStorageDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(mDevice, mStorageDescriptorSetLayout, VK_NULL_HANDLE);
  if (mUniformDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(mDevice, mUniformDescriptorSetLayout, VK_NULL_HANDLE);
  if (mTextureDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(mDevice, mTextureDescriptorSetLayout, VK_NULL_HANDLE);
  if (mDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(mDevice, mDescriptorPool, VK_NULL_HANDLE);

  if (mFence != VK_NULL_HANDLE) vkDestroyFence(mDevice, mFence, VK_NULL_HANDLE);
  if (mCommandBuffer != VK_NULL_HANDLE) vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mCommandBuffer);
  if (mCommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(mDevice, mCommandPool, VK_NULL_HANDLE);

  if (mDevice != VK_NULL_HANDLE) vkDestroyDevice(mDevice, VK_NULL_HANDLE);
  if (mInstance != VK_NULL_HANDLE) vkDestroyInstance(mInstance, VK_NULL_HANDLE);
}

purrr::Window *Context::createWindow(const WindowInfo &info) {
  return new Window(this, info);
}

purrr::Buffer *Context::createBuffer(const BufferInfo &info) {
  return new Buffer(this, info);
}

purrr::Shader *Context::createShader(const ShaderInfo &info) {
  return new Shader(this, info);
}

purrr::Sampler *Context::createSampler(const SamplerInfo &info) {
  return new Sampler(this, info);
}

purrr::Image *Context::createImage(const ImageInfo &info) {
  return new Image(this, info);
}

purrr::RenderTarget *Context::createRenderTarget(const RenderTargetInfo &info) {
  return new RenderTarget(this, info);
}

purrr::Shader *Context::createShader(ShaderType type, const std::vector<char> &code) {
  return new Shader(this, ShaderInfo{ .type = type, .code = code.data(), .codeLength = code.size() });
}

purrr::Shader *Context::createShader(ShaderType type, const std::string_view &code) {
  return new Shader(this, ShaderInfo{ .type = type, .code = code.data(), .codeLength = code.size() });
}

void Context::begin() {
  expectResult("Wait for fence", vkWaitForFences(mDevice, 1, &mFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
  expectResult("Fence reset", vkResetFences(mDevice, 1, &mFence));

  expectResult("Command buffer reset", vkResetCommandBuffer(mCommandBuffer, 0));

  auto beginInfo = VkCommandBufferBeginInfo{ .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                             .pNext            = VK_NULL_HANDLE,
                                             .flags            = 0,
                                             .pInheritanceInfo = VK_NULL_HANDLE };

  expectResult("Command buffer begin", vkBeginCommandBuffer(mCommandBuffer, &beginInfo));
}

bool Context::record(purrr::Window *window, const RecordClear &clear) {
  if (window->api() != api()) return false;
  // TODO: Introduce InvalidUse exception
  if (mRecording) throw std::runtime_error("Cannot record before calling end()");

  Window *vkWindow = reinterpret_cast<Window *>(window);
  if (!vkWindow->sameContext(this)) return false;

  uint32_t imageIndex = 0;
  VkResult result     = VK_SUCCESS;

  result = vkAcquireNextImageKHR(
      mDevice,
      vkWindow->getSwapchain(),
      std::numeric_limits<uint64_t>::max(),
      vkWindow->getImageSemaphore(),
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    mRecreateQueue.push(vkWindow);
    return false;
  } else if (result != VK_SUBOPTIMAL_KHR)
    expectResult("Next image acquire", result);

  mRecording = true;
  mWindows.push_back(vkWindow);
  mSwapchains.push_back(vkWindow->getSwapchain());
  mImageIndices.push_back(imageIndex);
  mImageSemaphores.push_back(vkWindow->getImageSemaphore());
  mSubmitSemaphores.push_back(vkWindow->getSubmitSemaphores()[imageIndex]);

  auto clearValues = std::vector<VkClearValue>();
  for (const ContextClearValue &value : clear.clearValues) {
    clearValues.push_back(*reinterpret_cast<const VkClearValue *>(&value));
  }

  auto renderPassBeginInfo =
      VkRenderPassBeginInfo{ .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                             .pNext           = VK_NULL_HANDLE,
                             .renderPass      = vkWindow->getRenderPass(),
                             .framebuffer     = vkWindow->getFramebuffers()[imageIndex],
                             .renderArea      = VkRect2D{ .offset = {}, .extent = vkWindow->getSwapchainExtent() },
                             .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                             .pClearValues    = clearValues.data() };

  vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  auto size = vkWindow->getSize();

  auto viewport = VkViewport{ .x        = 0.0f,
                              .y        = 0.0f,
                              .width    = static_cast<float>(size.first),
                              .height   = static_cast<float>(size.second),
                              .minDepth = 0.0f,
                              .maxDepth = 1.0f };

  vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mCommandBuffer, 0, 1, &renderPassBeginInfo.renderArea);

  return true;
}

bool Context::record(purrr::RenderTarget *target, const RecordClear &clear) {
  if (target->api() != api()) return false;
  // TODO: Introduce InvalidUse exception
  if (mRecording) throw std::runtime_error("Cannot record before calling end()");

  RenderTarget *vkTarget = reinterpret_cast<RenderTarget *>(target);
  if (!vkTarget->sameContext(this)) return false;

  mRecording = true;

  auto clearValues = std::vector<VkClearValue>();
  for (const ContextClearValue &value : clear.clearValues) {
    clearValues.push_back(*reinterpret_cast<const VkClearValue *>(&value));
  }

  auto size = vkTarget->getSize();

  auto renderPassBeginInfo =
      VkRenderPassBeginInfo{ .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                             .pNext       = VK_NULL_HANDLE,
                             .renderPass  = vkTarget->getRenderPass(),
                             .framebuffer = vkTarget->getFramebuffer(),
                             .renderArea =
                                 VkRect2D{ .offset = {},
                                           .extent = VkExtent2D{ .width  = static_cast<uint32_t>(size.first),
                                                                 .height = static_cast<uint32_t>(size.second) } },
                             .clearValueCount = static_cast<uint32_t>(clearValues.size()),
                             .pClearValues    = clearValues.data() };

  vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  auto viewport = VkViewport{ .x        = 0.0f,
                              .y        = 0.0f,
                              .width    = static_cast<float>(size.first),
                              .height   = static_cast<float>(size.second),
                              .minDepth = 0.0f,
                              .maxDepth = 1.0f };

  vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mCommandBuffer, 0, 1, &renderPassBeginInfo.renderArea);

  return true;
}

void Context::useProgram(purrr::Program *program) {
  if (!mRecording) throw std::runtime_error("useProgram() called before record()");

  if (program->api() != Api::Vulkan) throw std::runtime_error("Uncompatible program object");
  Program *vkProgram = reinterpret_cast<Program *>(program);
  if (!vkProgram->sameRenderTarget(mWindows.back())) throw std::runtime_error("Uncompatible program object");
  mProgram = vkProgram;

  vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkProgram->getPipeline());
}

void Context::useVertexBuffer(purrr::Buffer *buffer, uint32_t index) {
  if (!mRecording) throw std::runtime_error("useVertexBuffer() called before record()");

  if (buffer->api() != Api::Vulkan) throw std::runtime_error("Uncompatible buffer object");
  Buffer *vkBuffer = reinterpret_cast<Buffer *>(buffer);
  if (vkBuffer->getType() != BufferType::Vertex) throw std::runtime_error("Uncompatible buffer object");

  VkBuffer     buffers[1] = { vkBuffer->getBuffer() };
  VkDeviceSize offsets[1] = { 0 };
  vkCmdBindVertexBuffers(mCommandBuffer, index, 1, buffers, offsets);
}

void Context::useIndexBuffer(purrr::Buffer *buffer, IndexType type) {
  if (!mRecording) throw std::runtime_error("useIndexBuffer() called before record()");

  if (buffer->api() != Api::Vulkan) throw std::runtime_error("Uncompatible buffer object");
  Buffer *vkBuffer = reinterpret_cast<Buffer *>(buffer);
  if (vkBuffer->getType() != BufferType::Index) throw std::runtime_error("Uncompatible buffer object");

  vkCmdBindIndexBuffer(mCommandBuffer, vkBuffer->getBuffer(), 0, vkIndexType(type));
}

void Context::useUniformBuffer(purrr::Buffer *buffer, uint32_t index) {
  if (!mRecording) throw std::runtime_error("useUniformBuffer() called before record()");

  if (buffer->api() != Api::Vulkan) throw std::runtime_error("Uncompatible buffer object");
  Buffer *vkBuffer = reinterpret_cast<Buffer *>(buffer);
  if (vkBuffer->getType() != BufferType::Uniform) throw std::runtime_error("Uncompatible buffer object");

  VkDescriptorSet sets[1] = { vkBuffer->getDescriptorSet() };
  vkCmdBindDescriptorSets(
      mCommandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mProgram->getLayout(),
      index,
      1,
      sets,
      0,
      VK_NULL_HANDLE);
}

void Context::useStorageBuffer(purrr::Buffer *buffer, uint32_t index) {
  if (!mRecording) throw std::runtime_error("useStorageBuffer() called before record()");

  if (buffer->api() != Api::Vulkan) throw std::runtime_error("Uncompatible buffer object");
  Buffer *vkBuffer = reinterpret_cast<Buffer *>(buffer);
  if (vkBuffer->getType() != BufferType::Storage) throw std::runtime_error("Uncompatible buffer object");

  VkDescriptorSet sets[1] = { vkBuffer->getDescriptorSet() };
  vkCmdBindDescriptorSets(
      mCommandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mProgram->getLayout(),
      index,
      1,
      sets,
      0,
      VK_NULL_HANDLE);
}

void Context::useTextureImage(purrr::Image *image, uint32_t index) {
  if (!mRecording) throw std::runtime_error("useTextureImage() called before record()");
  if (!mProgram) throw std::runtime_error("useTextureImage() called before useProgram()");

  if (image->api() != Api::Vulkan) throw std::runtime_error("Uncompatible image object");
  Image *vkImage = reinterpret_cast<Image *>(image);
  if (!vkImage->getUsage().texture) throw std::runtime_error("Uncompatible image object");

  VkDescriptorSet sets[1] = { vkImage->getDescriptorSet() };
  vkCmdBindDescriptorSets(
      mCommandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      mProgram->getLayout(),
      index,
      1,
      sets,
      0,
      VK_NULL_HANDLE);
}

void Context::draw(size_t vertexCount, size_t instanceCount) {
  if (!mRecording) throw std::runtime_error("draw() called before record()");

  vkCmdDraw(mCommandBuffer, static_cast<uint32_t>(vertexCount), static_cast<uint32_t>(instanceCount), 0, 0);
}

void Context::drawIndexed(size_t indexCount, size_t instanceCount) {
  if (!mRecording) throw std::runtime_error("draw() called before record()");

  vkCmdDrawIndexed(mCommandBuffer, static_cast<uint32_t>(indexCount), static_cast<uint32_t>(instanceCount), 0, 0, 0);
}

void Context::end() {
  if (!mRecording) throw std::runtime_error("end() called before record()");
  mRecording = false;
  vkCmdEndRenderPass(mCommandBuffer);
}

void Context::submit() {
  vkEndCommandBuffer(mCommandBuffer);
  if (mRecording) throw std::runtime_error("Cannot submit while recording");

  auto stageMasks =
      std::vector<VkPipelineStageFlags>(mImageSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  auto submitInfo = VkSubmitInfo{ .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                  .pNext                = VK_NULL_HANDLE,
                                  .waitSemaphoreCount   = static_cast<uint32_t>(mImageSemaphores.size()),
                                  .pWaitSemaphores      = mImageSemaphores.data(),
                                  .pWaitDstStageMask    = stageMasks.data(),
                                  .commandBufferCount   = 1,
                                  .pCommandBuffers      = &mCommandBuffer,
                                  .signalSemaphoreCount = static_cast<uint32_t>(mSubmitSemaphores.size()),
                                  .pSignalSemaphores    = mSubmitSemaphores.data() };

  expectResult("Queue submition", vkQueueSubmit(mQueue, 1, &submitInfo, mFence));
}

void Context::present(bool preventSpinning) {
  if (mRecording) throw std::runtime_error("Cannot present while recording");

  std::vector<VkResult> results(mSwapchains.size(), VK_SUCCESS);

  auto presentInfo = VkPresentInfoKHR{ .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                       .pNext              = VK_NULL_HANDLE,
                                       .waitSemaphoreCount = static_cast<uint32_t>(mSubmitSemaphores.size()),
                                       .pWaitSemaphores    = mSubmitSemaphores.data(),
                                       .swapchainCount     = static_cast<uint32_t>(mSwapchains.size()),
                                       .pSwapchains        = mSwapchains.data(),
                                       .pImageIndices      = mImageIndices.data(),
                                       .pResults           = results.data() };

  if (!mSwapchains.empty()) {
    VkResult result = vkQueuePresentKHR(mQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      for (size_t i = 0; i < results.size(); ++i) {
        result = results[i];
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          mRecreateQueue.push(mWindows[i]);
        }
      }
    } else {
      expectResult("Presentation", result);
    }
  }

  if (!mRecreateQueue.empty()) {
    vkDeviceWaitIdle(mDevice);

    std::queue<Window *> recreateQueue = {};
    while (!mRecreateQueue.empty()) {
      Window *window = mRecreateQueue.back();
      mRecreateQueue.pop();
      if (!window->recreateSwapchain()) recreateQueue.push(window);
    }

    mRecreateQueue = recreateQueue;

    if (!mRecreateQueue.empty() && mWindows.empty() && preventSpinning) waitForWindowEvents();
  }

  mWindows.clear();
  mSwapchains.clear();
  mImageIndices.clear();
  mImageSemaphores.clear();
  mSubmitSemaphores.clear();
}

void Context::waitIdle() {
  expectResult("Wait idle", vkDeviceWaitIdle(mDevice));
}

void Context::createInstance(const ContextInfo &info) {
  auto layers     = std::vector<const char *>();
  auto extensions = std::vector<const char *>();

  appendRequiredVulkanExtensions(extensions);

  if (info.debug) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  { // Check if every required layer is present
    uint32_t count = 0;
    expectResult("Instance layer enumeration", vkEnumerateInstanceLayerProperties(&count, VK_NULL_HANDLE));

    auto availableLayers = std::vector<VkLayerProperties>(count);
    expectResult("Instance layer enumeration", vkEnumerateInstanceLayerProperties(&count, availableLayers.data()));

    auto requiredLayers = std::unordered_set<std::string_view>(layers.begin(), layers.end());
    for (const VkLayerProperties &layer : availableLayers) {
      requiredLayers.erase(layer.layerName);
    }

    if (!requiredLayers.empty()) throw NotPresent("layers", requiredLayers);
  }

  { // Check if every required extension is present
    uint32_t count = 0;
    expectResult(
        "Instance extension enumeration",
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &count, VK_NULL_HANDLE));

    auto availableExtensions = std::vector<VkExtensionProperties>(count);
    expectResult(
        "Instance extension enumeration",
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &count, availableExtensions.data()));

    auto requiredExtensions = std::unordered_set<std::string_view>(extensions.begin(), extensions.end());
    for (const VkExtensionProperties &extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) throw NotPresent("extensions", requiredExtensions);
  }

  auto applicationInfo = VkApplicationInfo{ .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                            .pNext              = VK_NULL_HANDLE,
                                            .pApplicationName   = info.appName,
                                            .applicationVersion = info.appVersion,
                                            .pEngineName        = info.engineName,
                                            .engineVersion      = info.engineVersion,
                                            .apiVersion         = info.apiVersion };

  auto createInfo = VkInstanceCreateInfo{ .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                          .pNext                   = VK_NULL_HANDLE,
                                          .flags                   = 0,
                                          .pApplicationInfo        = &applicationInfo,
                                          .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
                                          .ppEnabledLayerNames     = layers.data(),
                                          .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
                                          .ppEnabledExtensionNames = extensions.data() };

  expectResult("Instance creation", vkCreateInstance(&createInfo, VK_NULL_HANDLE, &mInstance));
}

void Context::chooseDevice(const std::vector<const char *> &extensions) {
  uint32_t count = 0;
  expectResult("Physical device enumeration", vkEnumeratePhysicalDevices(mInstance, &count, VK_NULL_HANDLE));

  auto devices = std::vector<VkPhysicalDevice>(count);
  expectResult("Physical device enumeration", vkEnumeratePhysicalDevices(mInstance, &count, devices.data()));

  /*
   0 - every device scored 0
   1 - no device that scored more than 0, had a graphics queue
   2 - no suitable device had all required extensions present
   */
  uint32_t found = 0;

  uint32_t bestScore = 0;
  for (VkPhysicalDevice device : devices) {
    uint32_t score = scorePhysicalDevice(device);
    if (score <= bestScore) continue;

    uint32_t queueFamilyIndex = findQueueFamily(device);
    if (queueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
      if (found < 1) found = 1;
      continue;
    }

    if (!deviceExtensionsPresent(device, extensions)) {
      found = 2;
      continue;
    }

    bestScore         = score;
    mPhysicalDevice   = device;
    mQueueFamilyIndex = queueFamilyIndex;
  }

  if (!mPhysicalDevice) {
    switch (found) {
    case 0:
    case 1: throw std::runtime_error("No suitable devices found");
    case 2: throw std::runtime_error("No suitable device had every required extension present");
    default: throw std::runtime_error("Something went wrong...");
    }
  }
}

void Context::createDevice(const std::vector<const char *> &extensions) {
  auto features = VkPhysicalDeviceFeatures{};

  float priorities      = 0.0f;
  auto  queueCreateInfo = VkDeviceQueueCreateInfo{ .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                   .pNext            = VK_NULL_HANDLE,
                                                   .flags            = 0,
                                                   .queueFamilyIndex = mQueueFamilyIndex,
                                                   .queueCount       = 1,
                                                   .pQueuePriorities = &priorities };

  auto createInfo = VkDeviceCreateInfo{ .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                        .pNext                   = VK_NULL_HANDLE,
                                        .flags                   = 0,
                                        .queueCreateInfoCount    = 1,
                                        .pQueueCreateInfos       = &queueCreateInfo,
                                        .enabledLayerCount       = 0,
                                        .ppEnabledLayerNames     = VK_NULL_HANDLE,
                                        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
                                        .ppEnabledExtensionNames = extensions.data(),
                                        .pEnabledFeatures        = &features };

  expectResult("Device creation", vkCreateDevice(mPhysicalDevice, &createInfo, VK_NULL_HANDLE, &mDevice));
}

void Context::getQueue() {
  vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);
}

void Context::createCommandPool() {
  auto createInfo = VkCommandPoolCreateInfo{ .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                             .pNext            = VK_NULL_HANDLE,
                                             .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                             .queueFamilyIndex = mQueueFamilyIndex };

  expectResult("Command pool creation", vkCreateCommandPool(mDevice, &createInfo, VK_NULL_HANDLE, &mCommandPool));
}

void Context::allocateCommandBuffer() {
  auto allocateInfo = VkCommandBufferAllocateInfo{ .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                   .pNext              = VK_NULL_HANDLE,
                                                   .commandPool        = mCommandPool,
                                                   .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                   .commandBufferCount = 1 };

  expectResult("Command buffer allocation", vkAllocateCommandBuffers(mDevice, &allocateInfo, &mCommandBuffer));
}

void Context::createFence() {
  auto createInfo = VkFenceCreateInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                       .pNext = VK_NULL_HANDLE,
                                       .flags = VK_FENCE_CREATE_SIGNALED_BIT };

  expectResult("Fence creation", vkCreateFence(mDevice, &createInfo, VK_NULL_HANDLE, &mFence));
}

void Context::createDescriptorSetLayouts() {
  { // Texture
    auto binding = VkDescriptorSetLayoutBinding{ .binding            = 0,
                                                 .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                 .descriptorCount    = 1,
                                                 .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                 .pImmutableSamplers = VK_NULL_HANDLE };

    auto createInfo = VkDescriptorSetLayoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .flags = 0,
                                                       .bindingCount = 1,
                                                       .pBindings    = &binding };

    expectResult(
        "Descriptor set layout creation",
        vkCreateDescriptorSetLayout(mDevice, &createInfo, VK_NULL_HANDLE, &mTextureDescriptorSetLayout));
  }

  { // Uniform
    auto binding =
        VkDescriptorSetLayoutBinding{ .binding            = 0,
                                      .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      .descriptorCount    = 1,
                                      .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                      .pImmutableSamplers = VK_NULL_HANDLE };

    auto createInfo = VkDescriptorSetLayoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .flags = 0,
                                                       .bindingCount = 1,
                                                       .pBindings    = &binding };

    expectResult(
        "Descriptor set layout creation",
        vkCreateDescriptorSetLayout(mDevice, &createInfo, VK_NULL_HANDLE, &mUniformDescriptorSetLayout));
  }

  { // Storage
    auto binding =
        VkDescriptorSetLayoutBinding{ .binding            = 0,
                                      .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                      .descriptorCount    = 1,
                                      .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                      .pImmutableSamplers = VK_NULL_HANDLE };

    auto createInfo = VkDescriptorSetLayoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                       .pNext = VK_NULL_HANDLE,
                                                       .flags = 0,
                                                       .bindingCount = 1,
                                                       .pBindings    = &binding };

    expectResult(
        "Descriptor set layout creation",
        vkCreateDescriptorSetLayout(mDevice, &createInfo, VK_NULL_HANDLE, &mStorageDescriptorSetLayout));
  }
}

void Context::createDescriptorPool() {
  auto poolSizes = std::array<VkDescriptorPoolSize, 3>(
      { VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1024 },
        VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1024 },
        VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024 } });

  auto createInfo = VkDescriptorPoolCreateInfo{ .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                .pNext         = VK_NULL_HANDLE,
                                                .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                                .maxSets       = 1024,
                                                .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                                                .pPoolSizes    = poolSizes.data() };

  expectResult(
      "Descriptor pool creation",
      vkCreateDescriptorPool(mDevice, &createInfo, VK_NULL_HANDLE, &mDescriptorPool));
}

uint32_t Context::scorePhysicalDevice(VkPhysicalDevice device) {
  uint32_t score = 0;

  auto properties = VkPhysicalDeviceProperties{};
  vkGetPhysicalDeviceProperties(device, &properties);

  switch (properties.deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
    score += 50;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
    score += 100;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
    score += 10;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_CPU: {
    score += 25;
    break;
  }
  default: break;
  }

  return score;
}

bool Context::deviceExtensionsPresent(VkPhysicalDevice device, const std::vector<const char *> extensions) {
  uint32_t count = 0;
  expectResult(
      "Device extension enumeration",
      vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &count, VK_NULL_HANDLE));

  auto availableExtensions = std::vector<VkExtensionProperties>(count);
  expectResult(
      "Device extension enumeration",
      vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &count, availableExtensions.data()));

  auto requiredExtensions = std::unordered_set<std::string_view>(extensions.begin(), extensions.end());
  for (const VkExtensionProperties &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

uint32_t Context::findQueueFamily(VkPhysicalDevice device) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);

  auto properties = std::vector<VkQueueFamilyProperties>(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

  for (uint32_t familyIndex = 0; familyIndex < properties.size(); ++familyIndex) {
    if (properties[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) return familyIndex;
  }

  return VK_QUEUE_FAMILY_IGNORED;
}

uint32_t Context::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  auto memoryProperties = VkPhysicalDeviceMemoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type");
}

VkCommandBuffer Context::beginSingleTimeCommands() {
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  auto allocateInfo = VkCommandBufferAllocateInfo{ .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                   .pNext              = VK_NULL_HANDLE,
                                                   .commandPool        = mCommandPool,
                                                   .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                   .commandBufferCount = 1 };

  expectResult("Command buffer allocation", vkAllocateCommandBuffers(mDevice, &allocateInfo, &commandBuffer));

  auto beginInfo = VkCommandBufferBeginInfo{ .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                             .pNext            = VK_NULL_HANDLE,
                                             .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                             .pInheritanceInfo = VK_NULL_HANDLE };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Context::submitSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  auto submitInfo = VkSubmitInfo{ .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                  .pNext                = VK_NULL_HANDLE,
                                  .waitSemaphoreCount   = 0,
                                  .pWaitSemaphores      = VK_NULL_HANDLE,
                                  .pWaitDstStageMask    = VK_NULL_HANDLE,
                                  .commandBufferCount   = 1,
                                  .pCommandBuffers      = &commandBuffer,
                                  .signalSemaphoreCount = 0,
                                  .pSignalSemaphores    = VK_NULL_HANDLE };

  vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(mQueue);

  vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}

} // namespace purrr::vulkan