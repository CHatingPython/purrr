#ifndef _PURRR_VULKAN_CONTEXT_HPP_
#define _PURRR_VULKAN_CONTEXT_HPP_

#include "purrr/context.hpp"
#include "purrr/object.hpp"

#include <utility>
#include <vector>
#include <vulkan/vulkan.h> // IWYU pragma: export

namespace purrr {
namespace vulkan {

  class Context : public purrr::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    Context(Context &&other) { *this = std::move(other); }

    Context &operator=(Context &&other) {
      if (this == &other) return *this;

      mInstance         = other.mInstance;
      mPhysicalDevice   = other.mPhysicalDevice;
      mQueueFamilyIndex = other.mQueueFamilyIndex;
      mDevice           = other.mDevice;
      mQueue            = other.mQueue;

      other.mInstance         = VK_NULL_HANDLE;
      other.mPhysicalDevice   = VK_NULL_HANDLE;
      other.mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      other.mDevice           = VK_NULL_HANDLE;
      other.mQueue            = VK_NULL_HANDLE;

      return *this;
    }
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
    void chooseDevice(const std::vector<const char *> &extensions);
    void createDevice(const std::vector<const char *> &extensions);
    void getQueue();
  private:
    virtual uint32_t scorePhysicalDevice(VkPhysicalDevice device);
    bool deviceExtensionsPresent(VkPhysicalDevice device, const std::vector<const char *> extensions);
  protected:
    uint32_t findQueueFamily(VkPhysicalDevice device);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_CONTEXT_HPP_