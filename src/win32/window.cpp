#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

#include <stdexcept>

namespace purrr::win32 {

Window::Window(Context *context, const WindowInfo &info)
  : mContext(context) {
  createWindow(info);
}

Window::~Window() {
  if (mWindowHandle) DestroyWindow(mWindowHandle);
}

void Window::setSize(const std::pair<int, int> &size) {
  RECT rect = { 0, 0, size.first, size.second };

  AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);

  SetWindowPos(
      mWindowHandle,
      HWND_TOP,
      0,
      0,
      rect.right - rect.left,
      rect.bottom - rect.top,
      SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
}

void Window::setPosition(const std::pair<int, int> &position) {
  RECT rect = { position.first, position.second, 0, 0 };
  AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);
  SetWindowPos(mWindowHandle, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

void Window::createWindow(const WindowInfo &info) {
  mStyle   = WS_OVERLAPPEDWINDOW;
  mExStyle = WS_EX_OVERLAPPEDWINDOW;

  RECT rect = { 0, 0, info.width, info.height };
  ::AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);

  int xPos = (info.xPos < 0) ? CW_USEDEFAULT : (info.xPos + rect.left);
  int yPos = (info.yPos < 0) ? CW_USEDEFAULT : (info.yPos + rect.top);

  int width  = (info.width < 0) ? CW_USEDEFAULT : (rect.right - rect.left);
  int height = (info.height < 0) ? CW_USEDEFAULT : (rect.bottom - rect.top);

  LPWSTR wideTitle = lpcstrToLpwstr(info.title);

  ATOM windowClass = mContext->getWindowClass();
  mWindowHandle    = CreateWindowExW(
      mExStyle,
      MAKEINTATOM(windowClass),
      wideTitle,
      mStyle,
      xPos,
      yPos,
      width,
      height,
      nullptr,
      nullptr,
      mContext->getInstance(),
      nullptr);

  delete[] wideTitle;

  if (mWindowHandle == nullptr) throw std::runtime_error("Failed to create a window");

  SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  ShowWindow(mWindowHandle, SW_SHOW);

  fetchPositionAndSize();
}

LPWSTR Window::lpcstrToLpwstr(LPCSTR cstr) {
  int length = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);

  LPWSTR wstr = new WCHAR[length];
  MultiByteToWideChar(CP_UTF8, 0, cstr, -1, wstr, length);
  wstr[length - 1] = L'\0';
  return wstr;
}

void Window::fetchPositionAndSize() {
  POINT pos = {};
  ClientToScreen(mWindowHandle, &pos);
  mXPos = pos.x;
  mYPos = pos.y;

  RECT area = {};
  GetClientRect(mWindowHandle, &area);
  mWidth  = area.right;
  mHeight = area.bottom;
}

LRESULT Window::windowProcedure(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam) {
  Window *window = reinterpret_cast<Window *>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));
  if (window == nullptr) return DefWindowProcW(windowHandle, msg, wParam, lParam);

  switch (msg) {
  case WM_CLOSE: {
    window->mShouldClose = true;
    return 0;
  }
  case WM_MOVE: {
    window->mXPos = LOWORD(lParam);
    window->mYPos = HIWORD(lParam);
    return 0;
  }
  case WM_SIZE: {
    window->mWidth  = LOWORD(lParam);
    window->mHeight = HIWORD(lParam);
    return 0;
  }
  default: return DefWindowProcW(windowHandle, msg, wParam, lParam);
  }
}

} // namespace purrr::win32