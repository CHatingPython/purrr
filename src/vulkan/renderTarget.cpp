#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/renderTarget.hpp"
#include "purrr/vulkan/program.hpp"
#include "purrr/vulkan/format.hpp"

#include <stdexcept>
#include <vector>

namespace purrr::vulkan {

RenderTarget::RenderTarget(Context *context, const RenderTargetInfo &info)
  : mContext(context) {
  mImages.reserve(info.imageCount);
  for (size_t i = 0; i < info.imageCount; ++i) {
    purrr::Image *image = info.images[i];
    if (image->api() != api()) throw std::runtime_error("Uncompatible image object");
    Image *vkImage = reinterpret_cast<Image *>(image);
    if (!vkImage->getUsage().renderTarget) throw std::runtime_error("Uncompatible image object");
    mImages.push_back(vkImage);
  }

  createRenderPass(info);
  createFramebuffer(info);
}

RenderTarget::~RenderTarget() {
  if (mFramebuffer) vkDestroyFramebuffer(mContext->getDevice(), mFramebuffer, VK_NULL_HANDLE);
  if (mRenderPass) vkDestroyRenderPass(mContext->getDevice(), mRenderPass, VK_NULL_HANDLE);
}

purrr::Program *RenderTarget::createProgram(const ProgramInfo &info) {
  return new Program(this, mContext, info);
}

void RenderTarget::createRenderPass(const RenderTargetInfo &info) {
  auto attachments    = std::vector<VkAttachmentDescription>(info.imageCount);
  auto attachmentRefs = std::vector<VkAttachmentReference>(info.imageCount);

  for (size_t i = 0; i < info.imageCount; ++i) {
    Image *image = mImages[i];

    attachments[i] = VkAttachmentDescription{ .flags          = 0,
                                              .format         = vkFormat(image->getFormat()),
                                              .samples        = VK_SAMPLE_COUNT_1_BIT,
                                              .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                              .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                              .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                              .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                              .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                              .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    attachmentRefs[i] = VkAttachmentReference{ .attachment = static_cast<uint32_t>(i),
                                               .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
  }

  auto subpass = VkSubpassDescription{ .flags                   = 0,
                                       .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       .inputAttachmentCount    = 0,
                                       .pInputAttachments       = VK_NULL_HANDLE,
                                       .colorAttachmentCount    = static_cast<uint32_t>(attachmentRefs.size()),
                                       .pColorAttachments       = attachmentRefs.data(),
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
                                            .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                            .pAttachments    = attachments.data(),
                                            .subpassCount    = 1,
                                            .pSubpasses      = &subpass,
                                            .dependencyCount = 1,
                                            .pDependencies   = &dependency };

  expectResult(
      "Render pass creation",
      vkCreateRenderPass(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mRenderPass));
}

void RenderTarget::createFramebuffer(const RenderTargetInfo &info) {
  auto attachments = std::vector<VkImageView>(info.imageCount);
  for (size_t i = 0; i < info.imageCount; ++i) {
    attachments[i] = mImages[i]->getImageView();
  }

  auto createInfo = VkFramebufferCreateInfo{ .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                             .pNext           = VK_NULL_HANDLE,
                                             .flags           = 0,
                                             .renderPass      = mRenderPass,
                                             .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                             .pAttachments    = attachments.data(),
                                             .width           = mWidth,
                                             .height          = mHeight,
                                             .layers          = 1 };

  expectResult(
      "Framebuffer creation",
      vkCreateFramebuffer(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mFramebuffer));
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN