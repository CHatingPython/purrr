#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/image.hpp"
#include "purrr/vulkan/buffer.hpp"
#include "purrr/vulkan/sampler.hpp"
#include "purrr/vulkan/format.hpp"

#include <stdexcept>
#include <cstring>
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

  VkBufferImageCopy region{};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.imageOffset       = {};
  region.imageExtent       = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

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

  VkImageCreateInfo createInfo{};
  createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  createInfo.pNext                 = VK_NULL_HANDLE;
  createInfo.flags                 = 0;
  createInfo.imageType             = VK_IMAGE_TYPE_2D;
  createInfo.format                = vkFormat(info.format);
  createInfo.extent                = { static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height), 1 };
  createInfo.mipLevels             = 1;
  createInfo.arrayLayers           = 1;
  createInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
  createInfo.tiling                = vkImageTiling(info.tiling);
  createInfo.usage                 = usage;
  createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE;
  createInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

  expectResult("Image creation", vkCreateImage(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImage));
}

void Image::allocateMemory() {
  VkMemoryRequirements memoryRequirements{};
  vkGetImageMemoryRequirements(mContext->getDevice(), mImage, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext          = VK_NULL_HANDLE;
  allocateInfo.allocationSize = memoryRequirements.size;
  allocateInfo.memoryTypeIndex =
      mContext->findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  expectResult("Memory allocation", vkAllocateMemory(mContext->getDevice(), &allocateInfo, nullptr, &mMemory));

  vkBindImageMemory(mContext->getDevice(), mImage, mMemory, 0);
}

void Image::createImageView(const ImageInfo &info) {
  VkImageViewCreateInfo createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.pNext            = VK_NULL_HANDLE;
  createInfo.flags            = 0;
  createInfo.image            = mImage;
  createInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format           = vkFormat(info.format);
  createInfo.components       = { VK_COMPONENT_SWIZZLE_R,
                                  VK_COMPONENT_SWIZZLE_G,
                                  VK_COMPONENT_SWIZZLE_B,
                                  VK_COMPONENT_SWIZZLE_A };
  createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

  expectResult(
      "Image view creation",
      vkCreateImageView(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mImageView));
}

void Image::allocateDescriptorSet(purrr::Sampler *sampler) {
  if (sampler->api() != Api::Vulkan) throw std::runtime_error("Uncompatible sampler object");
  Sampler *vkSampler = reinterpret_cast<Sampler *>(sampler);

  auto layout = mContext->getTextureDescriptorSetLayout();

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.pNext              = VK_NULL_HANDLE;
  allocateInfo.descriptorPool     = mContext->getDescriptorPool();
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts        = &layout;

  expectResult(
      "Descriptor set allocation",
      vkAllocateDescriptorSets(mContext->getDevice(), &allocateInfo, &mDescriptorSet));

  VkDescriptorImageInfo imageInfo{};
  imageInfo.sampler     = vkSampler->getSampler();
  imageInfo.imageView   = mImageView;
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write{};
  write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext            = VK_NULL_HANDLE;
  write.dstSet           = mDescriptorSet;
  write.dstBinding       = 0;
  write.dstArrayElement  = 0;
  write.descriptorCount  = 1;
  write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo       = &imageInfo;
  write.pBufferInfo      = VK_NULL_HANDLE;
  write.pTexelBufferView = VK_NULL_HANDLE;

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

  VkImageMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext               = VK_NULL_HANDLE;
  barrier.srcAccessMask       = mAccess;
  barrier.dstAccessMask       = dstAccess;
  barrier.oldLayout           = mLayout;
  barrier.newLayout           = dstLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image               = mImage;
  barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

  vkCmdPipelineBarrier(cmdBuf, mStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  if (commandBuffer == VK_NULL_HANDLE) {
    mContext->submitSingleTimeCommands(cmdBuf);
  }

  mLayout = dstLayout;
  mStage  = dstStage;
  mAccess = dstAccess;
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN