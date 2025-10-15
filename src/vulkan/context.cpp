#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/window.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <unordered_set>

#undef max

namespace purrr::vulkan {

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
}

Context::~Context() {
  if (mFence != VK_NULL_HANDLE) vkDestroyFence(mDevice, mFence, VK_NULL_HANDLE);
  if (mCommandBuffer != VK_NULL_HANDLE) vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mCommandBuffer);
  if (mCommandPool != VK_NULL_HANDLE) vkDestroyCommandPool(mDevice, mCommandPool, VK_NULL_HANDLE);

  if (mDevice != VK_NULL_HANDLE) vkDestroyDevice(mDevice, VK_NULL_HANDLE);
  if (mInstance != VK_NULL_HANDLE) vkDestroyInstance(mInstance, VK_NULL_HANDLE);
}

purrr::Window *Context::createWindow(const WindowInfo &info) {
  return new Window(this, info);
}

void Context::begin() {
  expectResult("Wait for fence", vkWaitForFences(mDevice, 1, &mFence, VK_TRUE, std::numeric_limits<uint64_t>::max()));
  expectResult("Fence reset", vkResetFences(mDevice, 1, &mFence));
}

bool Context::record(purrr::Window *window) {
  if (window->api() != api()) return false;
  // TODO: Introduce InvalidUse exception
  if (mRCommandBuffer != VK_NULL_HANDLE) throw std::runtime_error("Cannot record before calling end()");

  Window *vkWindow = reinterpret_cast<Window *>(window);
  if (!vkWindow->sameContext(this)) return false;

  mWindows.push_back(vkWindow);
  mRCommandBuffer = vkWindow->getCommandBuffer();
  mSwapchains.push_back(vkWindow->getSwapchain());

  uint32_t imageIndex = 0;
  VkResult result     = VK_SUCCESS;

  for (bool check = false;; check = true) {
    result = vkAcquireNextImageKHR(
        mDevice,
        vkWindow->getSwapchain(),
        std::numeric_limits<uint64_t>::max(),
        vkWindow->getImageSemaphore(),
        VK_NULL_HANDLE,
        &imageIndex);

    if (check || result != VK_ERROR_OUT_OF_DATE_KHR) break;
    // NOTE: Maybe make a queue of windows to recreate and recreate them in `present` or `begin`
    vkWindow->recreateSwapchain();
  }
  if (result != VK_SUBOPTIMAL_KHR) expectResult("Next image acquire", result);

  mImageIndices.push_back(imageIndex);

  expectResult("Command buffer reset", vkResetCommandBuffer(mRCommandBuffer, 0));

  auto beginInfo = VkCommandBufferBeginInfo{ .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                             .pNext            = VK_NULL_HANDLE,
                                             .flags            = 0,
                                             .pInheritanceInfo = VK_NULL_HANDLE };

  expectResult("Command buffer begin", vkBeginCommandBuffer(mRCommandBuffer, &beginInfo));

  auto clearValue = VkClearValue{ VkClearColorValue{ 1.0f, 0.0f, 0.0f, 1.0f } };

  auto renderPassBeginInfo =
      VkRenderPassBeginInfo{ .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                             .pNext           = VK_NULL_HANDLE,
                             .renderPass      = vkWindow->getRenderPass(),
                             .framebuffer     = vkWindow->getFramebuffers()[imageIndex],
                             .renderArea      = VkRect2D{ .offset = {}, .extent = vkWindow->getSwapchainExtent() },
                             .clearValueCount = 1,
                             .pClearValues    = &clearValue };

  vkCmdBeginRenderPass(mRCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  mImageSemaphores.push_back(vkWindow->getImageSemaphore());
  mSubmitSemaphores.push_back(vkWindow->getSubmitSemaphores()[imageIndex]);
  mCommandBuffers.push_back(mRCommandBuffer);

  return true;
}

void Context::end() {
  if (mRCommandBuffer == VK_NULL_HANDLE) throw std::runtime_error("end() called before record()");
  vkCmdEndRenderPass(mRCommandBuffer);
  vkEndCommandBuffer(mRCommandBuffer);

  mRCommandBuffer = VK_NULL_HANDLE;
}

void Context::submit() {
  if (mRCommandBuffer != VK_NULL_HANDLE) throw std::runtime_error("Cannot submit while recording");

  auto stageMasks =
      std::vector<VkPipelineStageFlags>(mImageSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  auto submitInfo = VkSubmitInfo{ .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                  .pNext                = VK_NULL_HANDLE,
                                  .waitSemaphoreCount   = static_cast<uint32_t>(mImageSemaphores.size()),
                                  .pWaitSemaphores      = mImageSemaphores.data(),
                                  .pWaitDstStageMask    = stageMasks.data(),
                                  .commandBufferCount   = static_cast<uint32_t>(mCommandBuffers.size()),
                                  .pCommandBuffers      = mCommandBuffers.data(),
                                  .signalSemaphoreCount = static_cast<uint32_t>(mSubmitSemaphores.size()),
                                  .pSignalSemaphores    = mSubmitSemaphores.data() };

  expectResult("Queue submition", vkQueueSubmit(mQueue, 1, &submitInfo, mFence));
}

void Context::present() {
  if (mRCommandBuffer != VK_NULL_HANDLE) throw std::runtime_error("Cannot present while recording");

  std::vector<VkResult> results(mSwapchains.size(), VK_SUCCESS);

  auto presentInfo = VkPresentInfoKHR{ .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                       .pNext              = VK_NULL_HANDLE,
                                       .waitSemaphoreCount = static_cast<uint32_t>(mSubmitSemaphores.size()),
                                       .pWaitSemaphores    = mSubmitSemaphores.data(),
                                       .swapchainCount     = static_cast<uint32_t>(mSwapchains.size()),
                                       .pSwapchains        = mSwapchains.data(),
                                       .pImageIndices      = mImageIndices.data(),
                                       .pResults           = results.data() };

  VkResult result = vkQueuePresentKHR(mQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    for (size_t i = 0; i < results.size(); ++i) {
      result = results[i];
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        mWindows[i]->recreateSwapchain();
      }
    }
  } else {
    expectResult("Presentation", result);
  }

  mWindows.clear();
  mSwapchains.clear();
  mImageIndices.clear();
  mImageSemaphores.clear();
  mSubmitSemaphores.clear();
  mCommandBuffers.clear();
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

} // namespace purrr::vulkan