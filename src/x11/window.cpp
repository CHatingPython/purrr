#ifdef _PURRR_PLATFORM_X11

#include "purrr/x11/context.hpp"
#include "purrr/x11/window.hpp"

#include <stdexcept>
#include <cassert>
#include <cstring>

#undef min

namespace purrr::platform::x11 {

Window::Window(Context *context, const WindowInfo &info)
  : mContext(context) {
  createWindow(info);
}

Window::~Window() {
  ::XDestroyWindow(mContext->getDisplay(), mWindow);
}

int Window::getTitle(char *title, int length) const {
  assert(title != nullptr);

  ::XTextProperty prop = {};
  if (!::XGetWMName(mContext->getDisplay(), mWindow, &prop)) return 0;

  if (!prop.value || prop.nitems == 0) return 0;

  int copyLen = std::min<int>(prop.nitems, length - 1);
  std::memcpy(title, prop.value, copyLen);
  title[copyLen] = '\0';

  ::XFree(prop.value);
  return copyLen;
}

void Window::setSize(const std::pair<int, int> &size) {
  ::XResizeWindow(mContext->getDisplay(), mWindow,
                  static_cast<unsigned int>(size.first),
                  static_cast<unsigned int>(size.second));
}

void Window::setPosition(const std::pair<int, int> &position) {
  ::XMoveWindow(mContext->getDisplay(), mWindow,
                position.first, position.second);
}

void Window::setTitle(const char *title, int titleLength) {
  if (titleLength == -1) titleLength = std::strlen(title);

  ::XChangeProperty(mContext->getDisplay(), mWindow,
                    mContext->getNetWmNameAtom(), mContext->getUtf8StringAtom(), 8,
                    PropModeReplace,
                    reinterpret_cast<const uint8_t*>(title), titleLength);

  ::XFlush(mContext->getDisplay());
}

void Window::createWindow(const WindowInfo &info) {
  Display *dpy    = mContext->getDisplay();
  int      screen = DefaultScreen(dpy);
  XWindow  root   = RootWindow(dpy, screen);

  mWindow = ::XCreateSimpleWindow(
    dpy,
    root,
    info.xPos,
    info.yPos,
    static_cast<unsigned int>(info.width),
    static_cast<unsigned int>(info.height),
    0,
    BlackPixel(dpy, screen),
    WhitePixel(dpy, screen)
  );

  if (!mWindow) throw std::runtime_error("Failed to create X11 window");

  ::XSaveContext(mContext->getDisplay(), mWindow, mContext->getContext(), (XPointer)this);

  mXPos   = info.xPos;
  mYPos   = info.yPos;
  mWidth  = info.width;
  mHeight = info.height;

  setTitle(info.title, info.titleLength);

  ::XSelectInput(dpy, mWindow, ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

  XAtom wmDeleteWindow = mContext->getDeleteWindowAtom();
  ::XSetWMProtocols(dpy, mWindow, &wmDeleteWindow, 1);

  ::XMapWindow(dpy, mWindow);
  ::XFlush(dpy);
}

#ifdef _PURRR_BACKEND_VULKAN
VkResult Window::createSurface(VkInstance instance, VkSurfaceKHR *surface) {
  auto createInfo = VkXlibSurfaceCreateInfoKHR{ .sType   = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                                                 .pNext  = VK_NULL_HANDLE,
                                                 .flags  = 0,
                                                 .dpy    = mContext->getDisplay(),
                                                 .window = mWindow };

  return vkCreateXlibSurfaceKHR(instance, &createInfo, VK_NULL_HANDLE, surface);
}
#endif

} // namespace purrr::platform::x11

#endif // _PURRR_PLATFORM_X11