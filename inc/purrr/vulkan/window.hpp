#ifndef _PURRR_VULKAN_WINDOW_HPP_
#define _PURRR_VULKAN_WINDOW_HPP_

#include "purrr/window.hpp"
#include "purrr/vulkan/context.hpp"

#include "purrr/platform.hpp"

#include <utility>

namespace purrr {
namespace vulkan {

  class Window : public purrr::platform::Window {
  public:
    Window(Context *context, const WindowInfo &info);
    ~Window();
  public:
    Window(Window &&other)
      : purrr::platform::Window((purrr::platform::Window &&)other) {
      *this = std::move(other);
    }

    Window &operator=(Window &&other) {
      if (this == &other) return *this;

      return *this;
    }
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    bool sameContext(Context *context) const { return mContext; }
  public:
    VkSurfaceKHR    getSurface() const { return mSurface; }
    VkFormat        getFormat() const { return mFormat; }
    VkColorSpaceKHR getColorSpace() const { return mColorSpace; }
    VkExtent2D      getSwapchainExtent() const { return mSwapchainExtent; }
    VkSwapchainKHR  getSwapchain() const { return mSwapchain; }
  public:
    const std::vector<VkSemaphore> &getSubmitSemaphores() const { return mSubmitSemaphores; }
    const VkSemaphore              &getImageSemaphore() const { return mImageSemaphore; }
    const VkCommandBuffer          &getCommandBuffer() const { return mCommandBuffer; }
  private:
    Context        *mContext         = nullptr;
    VkSurfaceKHR    mSurface         = VK_NULL_HANDLE;
    VkFormat        mFormat          = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mColorSpace      = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkCommandBuffer mCommandBuffer   = VK_NULL_HANDLE;
    VkExtent2D      mSwapchainExtent = {};
    VkSwapchainKHR  mSwapchain       = VK_NULL_HANDLE;
    uint32_t        mImageCount      = 0;
  private:
    std::vector<VkSemaphore> mSubmitSemaphores = {};
    VkSemaphore              mImageSemaphore   = VK_NULL_HANDLE;
  private:
    void chooseSurfaceFormat();
    void allocateCommandBuffer();
    void createSwapchain();
    void createSemaphores();
    void cleanupSwapchain();
  public:
    void recreateSwapchain();
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_WINDOW_HPP_