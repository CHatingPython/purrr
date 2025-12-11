#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/exceptions.hpp"

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/renderTarget.hpp"
#include "purrr/vulkan/program.hpp"
#include "purrr/vulkan/format.hpp"

#include <vector>

namespace purrr::vulkan {

RenderTarget::RenderTarget(Context *context, const RenderTargetInfo &info)
  : mContext(context) {
  mImages.reserve(info.imageCount);
  for (size_t i = 0; i < info.imageCount; ++i) {
    purrr::Image *image = info.images[i];
    if (image->api() != api()) throw InvalidUse("Uncompatible image object");
    Image *vkImage = reinterpret_cast<Image *>(image);
    if (!vkImage->getUsage().renderTarget) throw InvalidUse("Uncompatible image object");
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
  std::vector<VkAttachmentDescription> attachments(info.imageCount);
  std::vector<VkAttachmentReference>   attachmentRefs(info.imageCount);

  for (size_t i = 0; i < info.imageCount; ++i) {
    attachments[i].flags          = 0;
    attachments[i].format         = vkFormat(mImages[i]->getFormat());
    attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[i].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachmentRefs[i].attachment = static_cast<uint32_t>(i);
    attachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }

  VkSubpassDescription subpass{};
  subpass.flags                   = 0;
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount    = 0;
  subpass.pInputAttachments       = VK_NULL_HANDLE;
  subpass.colorAttachmentCount    = static_cast<uint32_t>(attachmentRefs.size());
  subpass.pColorAttachments       = attachmentRefs.data();
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
  createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  createInfo.pAttachments    = attachments.data();
  createInfo.subpassCount    = 1;
  createInfo.pSubpasses      = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies   = &dependency;

  expectResult(
      "Render pass creation",
      vkCreateRenderPass(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mRenderPass));
}

void RenderTarget::createFramebuffer(const RenderTargetInfo &info) {
  std::vector<VkImageView> attachments(info.imageCount);
  for (size_t i = 0; i < info.imageCount; ++i) {
    attachments[i] = mImages[i]->getImageView();
  }

  VkFramebufferCreateInfo createInfo{};
  createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  createInfo.pNext           = VK_NULL_HANDLE;
  createInfo.flags           = 0;
  createInfo.renderPass      = mRenderPass;
  createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  createInfo.pAttachments    = attachments.data();
  createInfo.width           = mWidth;
  createInfo.height          = mHeight;
  createInfo.layers          = 1;

  expectResult(
      "Framebuffer creation",
      vkCreateFramebuffer(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mFramebuffer));
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN