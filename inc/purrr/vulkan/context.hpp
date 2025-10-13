#ifndef _PURRR_VULKAN_CONTEXT_HPP_
#define _PURRR_VULKAN_CONTEXT_HPP_

#include "purrr/context.hpp"
#include "purrr/object.hpp"

#include <vulkan/vulkan.h> // IWYU pragma: export

namespace purrr {
namespace vulkan {

  class Context : public purrr::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    VkInstance       getInstance() const { return mInstance; }
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    uint32_t         getQueueFamilyIndex() const { return mQueueFamilyIndex; }
    VkDevice         getDevice() const { return mDevice; }
    VkQueue          getQueue() const { return mQueue; }
  private:
    VkInstance       mInstance         = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice   = VK_NULL_HANDLE;
    uint32_t         mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkDevice         mDevice           = VK_NULL_HANDLE;
    VkQueue          mQueue            = VK_NULL_HANDLE;
  private:
    void createInstance(const ContextInfo &info);
    void chooseDevice();
    void findQueueFamily();
    void createDevice();
    void getQueue();
  private:
    virtual uint32_t scorePhysicalDevice(VkPhysicalDevice device);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_CONTEXT_HPP_