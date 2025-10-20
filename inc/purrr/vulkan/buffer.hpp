#ifndef _PURRR_VULKAN_BUFFER_HPP_
#define _PURRR_VULKAN_BUFFER_HPP_

#include <vulkan/vulkan.h>

#include "purrr/buffer.hpp"

namespace purrr {
namespace vulkan {

  class Context;
  class Buffer : public purrr::Buffer {
  public:
    Buffer(Context *context, const BufferInfo &info);
    ~Buffer();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    virtual void copy(void *data, size_t offset, size_t size) override;
  public:
    BufferType     getType() const { return mType; }
    VkBuffer       getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }
  private:
    Context       *mContext = nullptr;
    BufferType     mType    = BufferType::Vertex;
    VkBuffer       mBuffer  = VK_NULL_HANDLE;
    VkDeviceMemory mMemory  = VK_NULL_HANDLE;
  public:
    static void createBuffer(
        Context           *context,
        VkDeviceSize       size,
        VkBufferUsageFlags usage,
        bool               deviceLocal,
        VkBuffer          *buffer,
        VkDeviceMemory    *memory);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_BUFFER_HPP_