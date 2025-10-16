#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/buffer.hpp"
#include "purrr/vulkan/context.hpp"

namespace purrr::vulkan {

Buffer::Buffer(Context *context, const BufferInfo &info)
  : mContext(context) {
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  switch (info.type) {
  case BufferType::Vertex: {
    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  } break;
  case BufferType::Index: {
    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  } break;
  }

  createBuffer(info.size, usage, true, &mBuffer, &mMemory);
}

Buffer::~Buffer() {
  if (mMemory) vkFreeMemory(mContext->getDevice(), mMemory, VK_NULL_HANDLE);
  if (mBuffer) vkDestroyBuffer(mContext->getDevice(), mBuffer, VK_NULL_HANDLE);
}

void Buffer::copy(void *data, size_t offset, size_t size) {
  VkBuffer       stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false, &stagingBuffer, &stagingMemory);

  void *stagingData = nullptr;
  expectResult("Mapping memory", vkMapMemory(mContext->getDevice(), stagingMemory, 0, size, 0, &stagingData));
  memcpy(stagingData, data, size);
  vkUnmapMemory(mContext->getDevice(), stagingMemory);

  VkCommandBuffer commandBuffer = mContext->beginSingleTimeCommands();

  auto copyRegion = VkBufferCopy{ .srcOffset = 0, .dstOffset = offset, .size = size };
  vkCmdCopyBuffer(commandBuffer, stagingBuffer, mBuffer, 1, &copyRegion);

  mContext->submitSingleTimeCommands(commandBuffer);
}

void Buffer::createBuffer(
    VkDeviceSize size, VkBufferUsageFlags usage, bool deviceLocal, VkBuffer *buffer, VkDeviceMemory *memory) {
  VkMemoryPropertyFlags memoryProperties =
      (deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                   : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

  auto createInfo = VkBufferCreateInfo{ .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                        .pNext                 = VK_NULL_HANDLE,
                                        .flags                 = 0,
                                        .size                  = size,
                                        .usage                 = usage,
                                        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                        .queueFamilyIndexCount = 0,
                                        .pQueueFamilyIndices   = VK_NULL_HANDLE };

  expectResult("Buffer creation", vkCreateBuffer(mContext->getDevice(), &createInfo, nullptr, buffer));

  auto memoryRequirements = VkMemoryRequirements{};
  vkGetBufferMemoryRequirements(mContext->getDevice(), *buffer, &memoryRequirements);

  auto allocateInfo =
      VkMemoryAllocateInfo{ .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                            .pNext          = VK_NULL_HANDLE,
                            .allocationSize = memoryRequirements.size,
                            .memoryTypeIndex =
                                mContext->findMemoryType(memoryRequirements.memoryTypeBits, memoryProperties) };

  expectResult("Memory allocation", vkAllocateMemory(mContext->getDevice(), &allocateInfo, nullptr, memory));

  vkBindBufferMemory(mContext->getDevice(), *buffer, *memory, 0);
}

} // namespace purrr::vulkan