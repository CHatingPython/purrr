#ifdef _PURRR_BACKEND_VULKAN

#ifndef _PURRR_VULKAN_WINDOW_HPP_
#define _PURRR_VULKAN_WINDOW_HPP_

#include "purrr/window.hpp"
#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/renderTarget.hpp"

#include "purrr/platform.hpp"

#include <utility>

namespace purrr {
namespace vulkan {

  class Window : public purrr::platform::Window, public IRenderTarget {
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
    virtual Api api() const override { return Api::Vulkan; }
  public:
    virtual purrr::Program *createProgram(const ProgramInfo &info) override;
  public:
    virtual std::pair<int, int> getSize() const override { return purrr::platform::Window::getSize(); }
  public:
    bool sameContext(Context *context) const { return context == mContext; }
  public:
    VkSurfaceKHR         getSurface() const { return mSurface; }
    VkFormat             getFormat() const { return mFormat; }
    VkColorSpaceKHR      getColorSpace() const { return mColorSpace; }
    virtual VkRenderPass getRenderPass() const override { return mRenderPass; }
    VkExtent2D           getSwapchainExtent() const { return mSwapchainExtent; }
    VkSwapchainKHR       getSwapchain() const { return mSwapchain; }
  public:
    const std::vector<VkImage>       &getImages() const { return mImages; }
    const std::vector<VkImageView>   &getImageViews() const { return mImageViews; }
    const std::vector<VkFramebuffer> &getFramebuffers() const { return mFramebuffers; }
  public:
    const std::vector<VkSemaphore> &getSubmitSemaphores() const { return mSubmitSemaphores; }
    const VkSemaphore              &getImageSemaphore() const { return mImageSemaphore; }
  private:
    Context        *mContext         = nullptr;
    VkSurfaceKHR    mSurface         = VK_NULL_HANDLE;
    VkFormat        mFormat          = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR mColorSpace      = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkRenderPass    mRenderPass      = VK_NULL_HANDLE;
    VkExtent2D      mSwapchainExtent = {};
    VkSwapchainKHR  mSwapchain       = VK_NULL_HANDLE;
    uint32_t        mImageCount      = 0;
  private:
    std::vector<VkImage>       mImages       = {};
    std::vector<VkImageView>   mImageViews   = {};
    std::vector<VkFramebuffer> mFramebuffers = {};
  private:
    std::vector<VkSemaphore> mSubmitSemaphores = {};
    VkSemaphore              mImageSemaphore   = VK_NULL_HANDLE;
  private:
    void chooseSurfaceFormat();
    void createRenderPass();
    void createSwapchain();
    void createImageViews();
    void createFramebuffers();
    void createSemaphores();
    void cleanupSwapchain();
  public:
    bool recreateSwapchain();
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_WINDOW_HPP_

#endif // _PURRR_BACKEND_VULKAN