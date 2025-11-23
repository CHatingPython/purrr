#ifdef _PURRR_PLATFORM_WIN32

#ifndef _PURRR_WIN32_CONTEXT_HPP_
#define _PURRR_WIN32_CONTEXT_HPP_

#include "purrr/context.hpp"

#define UNICODE
#define _UNICODE
#include <Windows.h>

#include <utility>
#include <vector>

#include "purrr/event.hpp"

namespace purrr::platform {
inline namespace win32 {

  class Context : public purrr::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    Context(Context &&other) { *this = std::move(other); }

    Context &operator=(Context &&other) {
      if (this == &other) return *this;

      mInstance    = other.mInstance;
      mWindowClass = other.mWindowClass;

      other.mInstance    = nullptr;
      other.mWindowClass = INVALID_ATOM;

      return *this;
    }
  public:
    virtual void   pollWindowEvents() const override;
    virtual void   waitForWindowEvents() const override;
    virtual double getTime() const override;
  public:
    HINSTANCE getInstance() const { return mInstance; }
    ATOM      getWindowClass() const { return mWindowClass; }
  private:
    HINSTANCE mInstance       = nullptr;
    ATOM      mWindowClass    = INVALID_ATOM;
    uint64_t  mTimerFrequency = 0;
  private:
    KeyCode mKeyCodes[512];
  private:
    void registerClass();
    void fillKeyCodeTable();
  protected:
#ifdef _PURRR_BACKEND_VULKAN
    void appendRequiredVulkanExtensions(std::vector<const char *> &extensions);
#endif
  public:
    KeyCode getKeyCode(WORD scanCode) const { return mKeyCodes[scanCode]; }
  };

} // namespace win32
} // namespace purrr::platform

#endif // _PURRR_WIN32_CONTEXT_HPP_

#endif // _PURRR_PLATFORM_WIN32