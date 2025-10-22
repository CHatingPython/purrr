#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/image.hpp"
#include "purrr/vulkan/buffer.hpp"
#include "purrr/vulkan/sampler.hpp"
#include "purrr/vulkan/format.hpp"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace purrr::vulkan {

VkImageTiling vkImageTiling(ImageTiling tiling) {
  switch (tiling) {
  case ImageTiling::Linear: return VK_IMAGE_TILING_LINEAR;
  case ImageTiling::Optimal: return VK_IMAGE_TILING_OPTIMAL;
  }

  throw std::runtime_error("Unreachable");
}

Image::Image(Context *context, const ImageInfo &info)
  : mContext(context), mUsage(info.usage) {
  createImage(info);
  allocateMemory();
  createImageView(info);
  if (info.usage.texture && info.sampler) allocateDescriptorSet(info.sampler);
}

Image::~Image() {
  if (mDescriptorSet) vkFreeDescriptorSets(mContext->getDevice(), mContext->getDescriptorPool(), 1, &mDescriptorSet);
  if (mImageView) vkDestroyImageView(mContext->getDevice(), mImageView, VK_NULL_HANDLE);
  if (mMemory) vkFreeMemory(mContext->getDevice(), mMemory, VK_NULL_HANDLE);
  if (mImage) vkDestroyImage(mContext->getDevice(), mImage, VK_NULL_HANDLE);
}

void Image::copyData(size_t width, size_t height, size_t size, const void *data) {
  VkBuffer       stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
  Buffer::createBuffer(mContext, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, &stagingBuffer, &stagingMemory);

  void *stagingData = nullptr;
  vkMapMemory(mContext->getDevice(), stagingMemory, 0, size, 0, &stagingData);
  memcpy(stagingData, data, size);
  vkUnmapMemory(mContext->getDevice(), stagingMemory);

  VkCommandBuffer commandBuffer = mContext->beginSingleTimeCommands();

  VkImageLayout        oldLayout = mLayout;
  VkPipelineStageFlags oldStage  = mStage;
  VkAccessFlags        oldAccess = mAccess;

  transitionImageLayout(
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      commandBuffer);

  auto region = VkBufferImageCopy{
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,
    .imageSubresource  = VkImageSubresourceLayers{ .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                   .mipLevel       = 0,
                                                   .baseArrayLayer = 0,
                                                   .layerCount     = 1 },
    .imageOffset       = VkOffset3D{},
    .imageExtent =
        VkExtent3D{ .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1 }
  };

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  transitionImageLayout(oldLayout, oldStage, oldAccess, commandBuffer);

  mContext->submitSingleTimeCommands(commandBuffer);

  vkFreeMemory(mContext->getDevice(), stagingMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(mContext->getDevice(), stagingBuffer, VK_NULL_HANDLE);
}

void Image::createImage(const ImageInfo &info) {
  VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  if (info.usage.texture) usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  if (info.usage.renderTarget) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto createInfo = VkImageCreateInfo{ .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                       .pNext                 = VK_NULL_HANDLE,
                                       .flags                 = 0,
                                       .imageType             = VK_IMAGE_TYPE_2D,
                                       .format                = vkFormat(info.format),
                                       .extent                = VkExtent3D{ .width  = static_cast<uint32_t>(info.width),
                                                                            .height = static_cast<uint32_t>(info.height),
                                                                            .depth  = 1 },
                                       .mipLevels             = 1,
                                       .arrayLayers           = 1,
                                       .samples               = VK_SAMPLE_COUNT_1_BIT,
                                       .tiling                = vkImageTiling(info.tiling),
                                       .usage                 = usage,
                                       .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                       .queueFamilyIndexCount = 0,
                                       .pQueueFamilyIndices   = VK_NULL_HANDLE,
                                       .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED };

  expectResult("Image creation", vkCreateImage(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImage));
}

void Image::allocateMemory() {
  auto memoryRequirements = VkMemoryRequirements{};
  vkGetImageMemoryRequirements(mContext->getDevice(), mImage, &memoryRequirements);

  auto allocateInfo = VkMemoryAllocateInfo{
    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext           = VK_NULL_HANDLE,
    .allocationSize  = memoryRequirements.size,
    .memoryTypeIndex = mContext->findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
  };

  expectResult("Memory allocation", vkAllocateMemory(mContext->getDevice(), &allocateInfo, nullptr, &mMemory));

  vkBindImageMemory(mContext->getDevice(), mImage, mMemory, 0);
}

void Image::createImageView(const ImageInfo &info) {
  auto createInfo =
      VkImageViewCreateInfo{ .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                             .pNext            = VK_NULL_HANDLE,
                             .flags            = 0,
                             .image            = mImage,
                             .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                             .format           = vkFormat(info.format),
                             .components       = VkComponentMapping{ .r = VK_COMPONENT_SWIZZLE_R,
                                                                     .g = VK_COMPONENT_SWIZZLE_G,
                                                                     .b = VK_COMPONENT_SWIZZLE_B,
                                                                     .a = VK_COMPONENT_SWIZZLE_A },
                             .subresourceRange = VkImageSubresourceRange{ .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                          .baseMipLevel   = 0,
                                                                          .levelCount     = 1,
                                                                          .baseArrayLayer = 0,
                                                                          .layerCount     = 1 } };

  expectResult(
      "Image view creation",
      vkCreateImageView(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImageView));
}

void Image::allocateDescriptorSet(purrr::Sampler *sampler) {
  if (sampler->api() != Api::Vulkan) throw std::runtime_error("Uncompatible sampler object");
  Sampler *vkSampler = reinterpret_cast<Sampler *>(sampler);

  auto layout = mContext->getTextureDescriptorSetLayout();

  auto allocateInfo = VkDescriptorSetAllocateInfo{ .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                   .pNext              = VK_NULL_HANDLE,
                                                   .descriptorPool     = mContext->getDescriptorPool(),
                                                   .descriptorSetCount = 1,
                                                   .pSetLayouts        = &layout };

  expectResult(
      "Descriptor set allocation",
      vkAllocateDescriptorSets(mContext->getDevice(), &allocateInfo, &mDescriptorSet));

  auto imageInfo = VkDescriptorImageInfo{ .sampler     = vkSampler->getSampler(),
                                          .imageView   = mImageView,
                                          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

  auto write = VkWriteDescriptorSet{ .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                     .pNext            = VK_NULL_HANDLE,
                                     .dstSet           = mDescriptorSet,
                                     .dstBinding       = 0,
                                     .dstArrayElement  = 0,
                                     .descriptorCount  = 1,
                                     .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     .pImageInfo       = &imageInfo,
                                     .pBufferInfo      = VK_NULL_HANDLE,
                                     .pTexelBufferView = VK_NULL_HANDLE };

  vkUpdateDescriptorSets(mContext->getDevice(), 1, &write, 0, VK_NULL_HANDLE);

  transitionImageLayout(
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_SHADER_READ_BIT);
}

void Image::transitionImageLayout(
    VkImageLayout dstLayout, VkPipelineStageFlags dstStage, VkAccessFlags dstAccess, VkCommandBuffer commandBuffer) {
  if (mLayout == dstLayout && mStage == dstStage && mAccess == dstAccess) return;

  VkCommandBuffer cmdBuf = commandBuffer;
  if (cmdBuf == VK_NULL_HANDLE) {
    cmdBuf = mContext->beginSingleTimeCommands();
  }

  auto barrier =
      VkImageMemoryBarrier{ .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                            .pNext               = VK_NULL_HANDLE,
                            .srcAccessMask       = mAccess,
                            .dstAccessMask       = dstAccess,
                            .oldLayout           = mLayout,
                            .newLayout           = dstLayout,
                            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                            .image               = mImage,
                            .subresourceRange    = VkImageSubresourceRange{ .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                            .baseMipLevel   = 0,
                                                                            .levelCount     = 1,
                                                                            .baseArrayLayer = 0,
                                                                            .layerCount     = 1 } };

  vkCmdPipelineBarrier(cmdBuf, mStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  if (commandBuffer == VK_NULL_HANDLE) {
    mContext->submitSingleTimeCommands(cmdBuf);
  }

  mLayout = dstLayout;
  mStage  = dstStage;
  mAccess = dstAccess;
}

} // namespace purrr::vulkan