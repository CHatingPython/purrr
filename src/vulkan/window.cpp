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
  createRenderPass();
  allocateCommandBuffer();
  createSwapchain();
}

Window::~Window() {
  cleanupSwapchain();

  if (mCommandBuffer) vkFreeCommandBuffers(mContext->getDevice(), mContext->getCommandPool(), 1, &mCommandBuffer);
  if (mRenderPass) vkDestroyRenderPass(mContext->getDevice(), mRenderPass, VK_NULL_HANDLE);
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

void Window::createRenderPass() {
  auto attachment = VkAttachmentDescription{ .flags          = 0,
                                             .format         = mFormat,
                                             .samples        = VK_SAMPLE_COUNT_1_BIT,
                                             .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                             .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                             .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                             .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                             .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                             .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

  auto attachmentReference =
      VkAttachmentReference{ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  auto subpass = VkSubpassDescription{ .flags                   = 0,
                                       .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       .inputAttachmentCount    = 0,
                                       .pInputAttachments       = VK_NULL_HANDLE,
                                       .colorAttachmentCount    = 1,
                                       .pColorAttachments       = &attachmentReference,
                                       .pResolveAttachments     = VK_NULL_HANDLE,
                                       .pDepthStencilAttachment = VK_NULL_HANDLE,
                                       .preserveAttachmentCount = 0,
                                       .pPreserveAttachments    = VK_NULL_HANDLE };

  auto dependency = VkSubpassDependency{ .srcSubpass      = VK_SUBPASS_EXTERNAL,
                                         .dstSubpass      = 0,
                                         .srcStageMask    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                         .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                         .srcAccessMask   = 0,
                                         .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         .dependencyFlags = 0 };

  auto createInfo = VkRenderPassCreateInfo{ .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                            .pNext           = VK_NULL_HANDLE,
                                            .flags           = 0,
                                            .attachmentCount = 1,
                                            .pAttachments    = &attachment,
                                            .subpassCount    = 1,
                                            .pSubpasses      = &subpass,
                                            .dependencyCount = 1,
                                            .pDependencies   = &dependency };

  expectResult(
      "Render pass creation",
      vkCreateRenderPass(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mRenderPass));
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

  auto createInfo =
      VkImageViewCreateInfo{ .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                             .pNext            = VK_NULL_HANDLE,
                             .flags            = 0,
                             .image            = VK_NULL_HANDLE,
                             .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                             .format           = mFormat,
                             .components       = VkComponentMapping{ .r = VK_COMPONENT_SWIZZLE_R,
                                                                     .g = VK_COMPONENT_SWIZZLE_G,
                                                                     .b = VK_COMPONENT_SWIZZLE_B,
                                                                     .a = VK_COMPONENT_SWIZZLE_A },
                             .subresourceRange = VkImageSubresourceRange{ .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                          .baseMipLevel   = 0,
                                                                          .levelCount     = 1,
                                                                          .baseArrayLayer = 0,
                                                                          .layerCount     = 1 } };

  for (const VkImage &image : mImages) {
    createInfo.image = image;

    expectResult(
        "Image view creation",
        vkCreateImageView(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImageViews.emplace_back()));
  }
}

void Window::createFramebuffers() {
  mFramebuffers.reserve(mImageCount);

  auto createInfo = VkFramebufferCreateInfo{ .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                             .pNext           = VK_NULL_HANDLE,
                                             .flags           = 0,
                                             .renderPass      = mRenderPass,
                                             .attachmentCount = 1,
                                             .pAttachments    = VK_NULL_HANDLE,
                                             .width           = mSwapchainExtent.width,
                                             .height          = mSwapchainExtent.height,
                                             .layers          = 1 };

  for (const VkImageView &imageView : mImageViews) {
    createInfo.pAttachments = &imageView;

    expectResult(
        "Framebuffer creation",
        vkCreateFramebuffer(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mFramebuffers.emplace_back()));
  }
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