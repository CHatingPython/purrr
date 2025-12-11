#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/window.hpp"
#include "purrr/vulkan/program.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#undef max

namespace purrr::vulkan {

Window::Window(Context *context, const WindowInfo &info)
  : purrr::platform::Window(context, info), mContext(context) {
  expectResult("Surface creation", createSurface(context->getInstance(), &mSurface));
  chooseSurfaceFormat();
  createRenderPass();
  createSwapchain();
}

Window::~Window() {
  cleanupSwapchain();

  if (mRenderPass) vkDestroyRenderPass(mContext->getDevice(), mRenderPass, VK_NULL_HANDLE);
  if (mSurface) vkDestroySurfaceKHR(mContext->getInstance(), mSurface, VK_NULL_HANDLE);
}

purrr::Program *Window::createProgram(const ProgramInfo &info) {
  return new Program(this, mContext, info);
}

void Window::chooseSurfaceFormat() {
  uint32_t count = 0;
  expectResult(
      "Surface formats query",
      vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->getPhysicalDevice(), mSurface, &count, VK_NULL_HANDLE));

  std::vector<VkSurfaceFormatKHR> availableFormats(count);
  expectResult(
      "Surface formats query",
      vkGetPhysicalDeviceSurfaceFormatsKHR(mContext->getPhysicalDevice(), mSurface, &count, availableFormats.data()));

  for (const VkSurfaceFormatKHR &format : availableFormats) {
    if ((format.format == VK_FORMAT_B8G8R8A8_UNORM || format.format == VK_FORMAT_R8G8B8A8_UNORM) &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      mFormat     = format.format;
      mColorSpace = format.colorSpace;
      return;
    }
  }

  throw std::runtime_error("Failed to choose a surface format");
}

void Window::createRenderPass() {
  VkAttachmentDescription attachment{};
  attachment.flags          = 0;
  attachment.format         = mFormat;
  attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference attachmentReference{};
  attachmentReference.attachment = 0;
  attachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.flags                   = 0;
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount    = 0;
  subpass.pInputAttachments       = VK_NULL_HANDLE;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &attachmentReference;
  subpass.pResolveAttachments     = VK_NULL_HANDLE;
  subpass.pDepthStencilAttachment = VK_NULL_HANDLE;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments    = VK_NULL_HANDLE;

  VkSubpassDependency dependency{};
  dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass      = 0;
  dependency.srcStageMask    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask   = 0;
  dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dependencyFlags = 0;

  VkRenderPassCreateInfo createInfo{};
  createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.pNext           = VK_NULL_HANDLE;
  createInfo.flags           = 0;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments    = &attachment;
  createInfo.subpassCount    = 1;
  createInfo.pSubpasses      = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies   = &dependency;

  expectResult(
      "Render pass creation",
      vkCreateRenderPass(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mRenderPass));
}

void Window::createSwapchain() {
  VkSurfaceCapabilitiesKHR capabilities{};
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

  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount)
    minImageCount = capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.pNext                 = VK_NULL_HANDLE;
  createInfo.flags                 = 0;
  createInfo.surface               = mSurface;
  createInfo.minImageCount         = minImageCount;
  createInfo.imageFormat           = mFormat;
  createInfo.imageColorSpace       = mColorSpace;
  createInfo.imageExtent           = mSwapchainExtent;
  createInfo.imageArrayLayers      = 1;
  createInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE;
  createInfo.preTransform          = capabilities.currentTransform;
  createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
  createInfo.clipped               = VK_TRUE;
  createInfo.oldSwapchain          = VK_NULL_HANDLE;

  expectResult(
      "Swapchain creation",
      vkCreateSwapchainKHR(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mSwapchain));

  expectResult(
      "Swapchain images query",
      vkGetSwapchainImagesKHR(mContext->getDevice(), mSwapchain, &mImageCount, VK_NULL_HANDLE));
  mImages.resize(mImageCount);
  expectResult(
      "Swapchain images query",
      vkGetSwapchainImagesKHR(mContext->getDevice(), mSwapchain, &mImageCount, mImages.data()));

  createImageViews();
  createFramebuffers();
  createSemaphores();
}

void Window::createImageViews() {
  mImageViews.reserve(mImageCount);

  VkImageViewCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.pNext            = VK_NULL_HANDLE;
  createInfo.flags            = 0;
  createInfo.image            = VK_NULL_HANDLE;
  createInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format           = mFormat;
  createInfo.components       = { VK_COMPONENT_SWIZZLE_R,
                                  VK_COMPONENT_SWIZZLE_G,
                                  VK_COMPONENT_SWIZZLE_B,
                                  VK_COMPONENT_SWIZZLE_A };
  createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

  for (const VkImage &image : mImages) {
    createInfo.image = image;

    expectResult(
        "Image view creation",
        vkCreateImageView(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImageViews.emplace_back()));
  }
}

void Window::createFramebuffers() {
  mFramebuffers.reserve(mImageCount);

  VkFramebufferCreateInfo createInfo{};
  createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  createInfo.pNext           = VK_NULL_HANDLE;
  createInfo.flags           = 0;
  createInfo.renderPass      = mRenderPass;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments    = VK_NULL_HANDLE;
  createInfo.width           = mSwapchainExtent.width;
  createInfo.height          = mSwapchainExtent.height;
  createInfo.layers          = 1;

  for (const VkImageView &imageView : mImageViews) {
    createInfo.pAttachments = &imageView;

    expectResult(
        "Framebuffer creation",
        vkCreateFramebuffer(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mFramebuffers.emplace_back()));
  }
}

void Window::createSemaphores() {
  VkSemaphoreCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  createInfo.pNext = VK_NULL_HANDLE;
  createInfo.flags = 0;

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

  for (VkFramebuffer framebuffer : mFramebuffers) {
    vkDestroyFramebuffer(mContext->getDevice(), framebuffer, VK_NULL_HANDLE);
  }
  mFramebuffers.clear();

  for (VkImageView imageView : mImageViews) {
    vkDestroyImageView(mContext->getDevice(), imageView, VK_NULL_HANDLE);
  }
  mImageViews.clear();

  mImages.clear();

  if (mSwapchain) vkDestroySwapchainKHR(mContext->getDevice(), mSwapchain, VK_NULL_HANDLE);
}

bool Window::recreateSwapchain() {
  auto [width, height] = getSize();
  if (width == 0 || height == 0) return false;

  cleanupSwapchain();
  createSwapchain();

  return true;
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN