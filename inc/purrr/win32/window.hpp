#ifndef _PURRR_WIN32_WINDOW_HPP_
#define _PURRR_WIN32_WINDOW_HPP_

#include "purrr/window.hpp"

#include <utility>

#include <Windows.h>

namespace purrr {
namespace win32 {

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

      mWidth        = other.mWidth;
      mHeight       = other.mHeight;
      mWindowHandle = other.mWindowHandle;

      other.mWidth        = 0;
      other.mHeight       = 0;
      other.mWindowHandle = nullptr;

      return *this;
    }
  public:
    virtual std::pair<int, int> getSize() const override { return std::make_pair(mWidth, mHeight); }
    virtual std::pair<int, int> getPosition() const override { return std::make_pair(mXPos, mYPos); }
    virtual bool                shouldClose() const override { return mShouldClose; }
  public:
    virtual void setSize(const std::pair<int, int> &size) override;
    virtual void setPosition(const std::pair<int, int> &position) override;
    virtual void shouldClose(bool shouldClose) override { mShouldClose = shouldClose; }
  private:
    Context *mContext = nullptr;
    WORD     mWidth = 0, mHeight = 0;
    WORD     mXPos = 0, mYPos = 0;
    bool     mShouldClose  = false;
    HWND     mWindowHandle = nullptr;
    DWORD    mStyle = 0, mExStyle = 0;
  private:
    void   createWindow(const WindowInfo &info);
    LPWSTR lpcstrToLpwstr(LPCSTR cstr);
    void   fetchPositionAndSize();
  private:
    static LRESULT windowProcedure(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam);
  };

} // namespace win32
} // namespace purrr

#endif // _PURRR_WIN32_WINDOW_HPP_