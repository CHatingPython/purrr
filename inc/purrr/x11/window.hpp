#ifdef _PURRR_PLATFORM_X11

#ifndef _PURRR_X11_WINDOW_HPP_
#define _PURRR_X11_WINDOW_HPP_

#include "purrr/window.hpp"

#include <utility>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef _PURRR_BACKEND_VULKAN
#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#endif

namespace purrr::platform {
namespace x11 {

  class Context;
  class Window : public purrr::Window {
    friend class Context;
  public:
    Window(Context *context, const WindowInfo &info);
    ~Window();
  public:
    Window(Window &&other) { *this = std::move(other); }

    Window &operator=(Window &&other) {
      if (this == &other) return *this;

      mWidth = other.mWindow;

      other.mWindow = 0;

      return *this;
    }
  public:
    virtual std::pair<int, int> getSize() const override { return std::make_pair(mWidth, mHeight); }
    virtual std::pair<int, int> getPosition() const override { return std::make_pair(mXPos, mYPos); }
  public:
    virtual int getTitle(char *title, int length) const override;
  public:
    virtual void setSize(const std::pair<int, int> &size) override;
    virtual void setPosition(const std::pair<int, int> &position) override;
    virtual void setTitle(const char *title, int titleLength) override;
  private:
    Context *mContext = nullptr;
    int      mWidth   = 0, mHeight = 0;
    int      mXPos    = 0, mYPos = 0;
    XWindow  mWindow  = 0;
  private:
    void createWindow(const WindowInfo &info);
  protected:
#ifdef _PURRR_BACKEND_VULKAN
    VkResult createSurface(VkInstance instance, VkSurfaceKHR *surface);
#endif
  };

} // namespace x11
} // namespace purrr::platform

#endif // _PURRR_X11_WINDOW_HPP_

#endif // _PURRR_PLATFORM_X11