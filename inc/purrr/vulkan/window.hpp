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
    VkSurfaceKHR    getSurface() const { return mSurface; }
    VkFormat        getFormat() const { return mFormat; }
    VkColorSpaceKHR getColorSpace() const { return mColorSpace; }
    VkSwapchainKHR  getSwapchain() const { return mSwapchain; }
  private:
    Context        *mContext         = nullptr;
    VkSurfaceKHR    mSurface         = VK_NULL_HANDLE;
    VkFormat        mFormat          = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mColorSpace      = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D      mSwapchainExtent = {};
    VkSwapchainKHR  mSwapchain       = VK_NULL_HANDLE;
  private:
    void chooseSurfaceFormat();
    void createSwapchain();
    void cleanupSwapchain();
    void recreateSwapchain();
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_WINDOW_HPP_