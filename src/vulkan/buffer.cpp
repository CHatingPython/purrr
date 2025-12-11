#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/buffer.hpp"
#include "purrr/vulkan/context.hpp"

#include <cstring>

namespace purrr::vulkan {

Buffer::Buffer(Context *context, const BufferInfo &info)
  : mContext(context), mSize(info.size), mType(info.type) {
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VkDescriptorType      descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  VkDescriptorSetLayout layout         = VK_NULL_HANDLE;

  switch (info.type) {
  case BufferType::Vertex: {
    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  } break;
  case BufferType::Index: {
    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  } break;
  case BufferType::Uniform: {
    usage         |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout         = mContext->getUniformDescriptorSetLayout();
  } break;
  case BufferType::Storage: {
    usage         |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layout         = mContext->getStorageDescriptorSetLayout();
  } break;
  }

  createBuffer(mContext, info.size, usage, true, &mBuffer, &mMemory);
  if (layout != VK_NULL_HANDLE) {
    allocateDescriptorSet(descriptorType, layout);
  }
}

Buffer::~Buffer() {
  if (mDescriptorSet) vkFreeDescriptorSets(mContext->getDevice(), mContext->getDescriptorPool(), 1, &mDescriptorSet);
  if (mMemory) vkFreeMemory(mContext->getDevice(), mMemory, VK_NULL_HANDLE);
  if (mBuffer) vkDestroyBuffer(mContext->getDevice(), mBuffer, VK_NULL_HANDLE);
}

void Buffer::copy(const void *data, size_t offset, size_t size) {
  VkBuffer       stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
  createBuffer(mContext, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, &stagingBuffer, &stagingMemory);

  void *stagingData = nullptr;
  expectResult("Mapping memory", vkMapMemory(mContext->getDevice(), stagingMemory, 0, size, 0, &stagingData));
  memcpy(stagingData, data, size);
  vkUnmapMemory(mContext->getDevice(), stagingMemory);

  VkCommandBuffer commandBuffer = mContext->beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = offset;
  copyRegion.size      = size;
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, mBuffer, 1, &copyRegion);

  mContext->submitSingleTimeCommands(commandBuffer);

  vkFreeMemory(mContext->getDevice(), stagingMemory, VK_NULL_HANDLE);
  vkDestroyBuffer(mContext->getDevice(), stagingBuffer, VK_NULL_HANDLE);
}

void Buffer::allocateDescriptorSet(VkDescriptorType type, VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.pNext              = VK_NULL_HANDLE;
  allocateInfo.descriptorPool     = mContext->getDescriptorPool();
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts        = &layout;

  expectResult(
      "Descriptor set allocation",
      vkAllocateDescriptorSets(mContext->getDevice(), &allocateInfo, &mDescriptorSet));

  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = mBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range  = mSize;

  VkWriteDescriptorSet write{};
  write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext            = VK_NULL_HANDLE;
  write.dstSet           = mDescriptorSet;
  write.dstBinding       = 0;
  write.dstArrayElement  = 0;
  write.descriptorCount  = 1;
  write.descriptorType   = type;
  write.pImageInfo       = VK_NULL_HANDLE;
  write.pBufferInfo      = &bufferInfo;
  write.pTexelBufferView = VK_NULL_HANDLE;

  vkUpdateDescriptorSets(mContext->getDevice(), 1, &write, 0, VK_NULL_HANDLE);
}

void Buffer::createBuffer(
    Context           *context,
    VkDeviceSize       size,
    VkBufferUsageFlags usage,
    bool               deviceLocal,
    VkBuffer          *buffer,
    VkDeviceMemory    *memory) {
  VkMemoryPropertyFlags memoryProperties =
      (deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                   : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

  VkBufferCreateInfo createInfo{};
  createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  createInfo.pNext                 = VK_NULL_HANDLE;
  createInfo.flags                 = 0;
  createInfo.size                  = size;
  createInfo.usage                 = usage;
  createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices   = VK_NULL_HANDLE;

  expectResult("Buffer creation", vkCreateBuffer(context->getDevice(), &createInfo, nullptr, buffer));

  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(context->getDevice(), *buffer, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext           = VK_NULL_HANDLE;
  allocateInfo.allocationSize  = memoryRequirements.size;
  allocateInfo.memoryTypeIndex = context->findMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

  expectResult("Memory allocation", vkAllocateMemory(context->getDevice(), &allocateInfo, nullptr, memory));

  vkBindBufferMemory(context->getDevice(), *buffer, *memory, 0);
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN