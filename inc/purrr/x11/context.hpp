#ifdef _PURRR_PLATFORM_X11

#ifndef _PURRR_X11_CONTEXT_HPP_
#define _PURRR_X11_CONTEXT_HPP_

#include "purrr/context.hpp"

#include <utility>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

namespace purrr::platform {
inline namespace x11 {

  using XDisplay = ::Display;
  using XWindow  = ::Window;
  using XAtom    = ::Atom;

  class Window;
  class Context : public purrr::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    Context(Context &&other) { *this = std::move(other); }

    Context &operator=(Context &&other) {
      if (this == &other) return *this;

      mDisplay = other.mDisplay;

      other.mDisplay = nullptr;

      return *this;
    }
  public:
    virtual void   pollWindowEvents() const override;
    virtual void   waitForWindowEvents() const override;
    virtual double getTime() const override;
  public:
    XDisplay *getDisplay() const { return mDisplay; }
    XContext  getContext() const { return mContext; }
    XAtom     getWmProtocolsAtom() const { return aWmProtocols; }
    XAtom     getDeleteWindowAtom() const { return aDeleteWindow; }
    XAtom     getNetWmNameAtom() const { return aNetWmName; }
    XAtom     getUtf8StringAtom() const { return aUtf8String; }
  private:
    XDisplay *mDisplay      = nullptr;
    XContext  mContext      = 0;
    XAtom     aWmProtocols  = 0;
    XAtom     aDeleteWindow = 0;
    XAtom     aNetWmName    = 0;
    XAtom     aUtf8String   = 0;
  private:
    KeyCode mKeyCodes[256];
  private:
    void fillKeyCodeTable();
  private:
    void handleEvent(::XEvent event) const;
  protected:
#ifdef _PURRR_BACKEND_VULKAN
    void appendRequiredVulkanExtensions(std::vector<const char *> &extensions);
#endif
  };

} // namespace x11
} // namespace purrr::platform

#endif // _PURRR_X11_CONTEXT_HPP_

#endif // _PURRR_PLATFORM_X11