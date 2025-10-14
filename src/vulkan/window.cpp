#include "purrr/vulkan/exceptions.hpp"
#include <algorithm>
#include <limits>
#include <vector>

#include "purrr/vulkan/window.hpp"

#undef max

namespace purrr::vulkan {

Window::Window(Context *context, const WindowInfo &info)
  : purrr::platform::Window(context, info), mContext(context) {
  expectResult("Surface creation", createSurface(context->getInstance(), &mSurface));
  chooseSurfaceFormat();
  allocateCommandBuffer();
  createSwapchain();
}

Window::~Window() {
  cleanupSwapchain();

  if (mCommandBuffer) vkFreeCommandBuffers(mContext->getDevice(), mContext->getCommandPool(), 1, &mCommandBuffer);
  if (mSurface) vkDestroySurfaceKHR(mContext->getInstance(), mSurface, VK_NULL_HANDLE);
}

void Window::chooseSurfaceFormat() {
  uint32_t count = 0;
  expectResult(
      "Surface formats query",
      vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->getPhysicalDevice(), mSurface, &count, VK_NULL_HANDLE));

  auto availableFormats = std::vector<VkSurfaceFormatKHR>(count);
  expectResult(
      "Surface formats query",
      vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->getPhysicalDevice(), mSurface, &count, availableFormats.data()));

  for (const VkSurfaceFormatKHR &format : availableFormats) {
    if ((format.format == VK_FORMAT_B8G8R8A8_SRGB || format.format == VK_FORMAT_R8G8B8A8_SRGB) &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      mFormat     = format.format;
      mColorSpace = format.colorSpace;
      return;
    }
  }

  throw std::runtime_error("Failed to choose a surface format");
}

void Window::allocateCommandBuffer() {
  auto allocateInfo = VkCommandBufferAllocateInfo{ .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                   .pNext              = VK_NULL_HANDLE,
                                                   .commandPool        = mContext->getCommandPool(),
                                                   .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                   .commandBufferCount = 1 };

  expectResult(
      "Command buffer allocation",
      vkAllocateCommandBuffers(mContext->getDevice(), &allocateInfo, &mCommandBuffer));
}

void Window::createSwapchain() {
  auto capabilities = VkSurfaceCapabilitiesKHR{};
  expectResult(
      "Surface capabilities query",
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext->getPhysicalDevice(), mSurface, &capabilities));

  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    mSwapchainExtent.width  = capabilities.currentExtent.width;
    mSwapchainExtent.height = capabilities.currentExtent.height;
  } else {
    auto [width, height] = getSize();
    mSwapchainExtent.width =
        std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    mSwapchainExtent.height = std::clamp(
        static_cast<uint32_t>(height),
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);
  }

  uint32_t minImageCount =
      std::clamp(capabilities.minImageCount + 1, capabilities.minImageCount, capabilities.maxImageCount);

  auto createInfo = VkSwapchainCreateInfoKHR{ .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                              .pNext                 = VK_NULL_HANDLE,
                                              .flags                 = 0,
                                              .surface               = mSurface,
                                              .minImageCount         = minImageCount,
                                              .imageFormat           = mFormat,
                                              .imageColorSpace       = mColorSpace,
                                              .imageExtent           = mSwapchainExtent,
                                              .imageArrayLayers      = 1,
                                              .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                              .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
                                              .queueFamilyIndexCount = 0,
                                              .pQueueFamilyIndices   = VK_NULL_HANDLE,
                                              .preTransform          = capabilities.currentTransform,
                                              .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                              .presentMode           = VK_PRESENT_MODE_FIFO_KHR,
                                              .clipped               = VK_TRUE,
                                              .oldSwapchain          = VK_NULL_HANDLE };

  expectResult(
      "Swapchain creation",
      vkCreateSwapchainKHR(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mSwapchain));

  expectResult(
      "Swapchain images query",
      vkGetSwapchainImagesKHR(mContext->getDevice(), mSwapchain, &mImageCount, VK_NULL_HANDLE));

  createSemaphores();
}

void Window::createSemaphores() {
  auto createInfo =
      VkSemaphoreCreateInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = VK_NULL_HANDLE, .flags = 0 };

  expectResult(
      "Semaphore creation",
      vkCreateSemaphore(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImageSemaphore));

  mSubmitSemaphores.resize(mImageCount);
  for (uint32_t i = 0; i < mImageCount; ++i) {
    expectResult(
        "Semaphore creation",
        vkCreateSemaphore(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mSubmitSemaphores[i]));
  }
}

void Window::cleanupSwapchain() {
  if (mImageSemaphore) vkDestroySemaphore(mContext->getDevice(), mImageSemaphore, VK_NULL_HANDLE);

  for (VkSemaphore semaphore : mSubmitSemaphores) {
    vkDestroySemaphore(mContext->getDevice(), semaphore, VK_NULL_HANDLE);
  }
  mSubmitSemaphores.clear();

  if (mSwapchain) vkDestroySwapchainKHR(mContext->getDevice(), mSwapchain, VK_NULL_HANDLE);
}

void Window::recreateSwapchain() {
  auto [width, height] = getSize();
  while (width == 0 && height == 0) {
    mContext->waitForWindowEvents();
    std::tie(width, height) = getSize();
  }

  vkDeviceWaitIdle(mContext->getDevice());

  cleanupSwapchain();
  createSwapchain();
}

} // namespace purrr::vulkan