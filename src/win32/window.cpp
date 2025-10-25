#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

#include <stdexcept>

#include <windowsx.h>

namespace purrr::win32 {

Window::Window(Context *context, const WindowInfo &info)
  : mContext(context) {
  createWindow(info);
  fetchPositionAndSize();

  {
    auto position = POINT{};
    GetCursorPos(&position);
    mCursorX = position.x;
    mCursorY = position.y;
  }
}

Window::~Window() {
  if (mWindowHandle) DestroyWindow(mWindowHandle);
}

void Window::setSize(const std::pair<int, int> &size) {
  auto rect = RECT{ 0, 0, size.first, size.second };

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
  auto rect = RECT{ position.first, position.second, 0, 0 };
  AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);
  SetWindowPos(mWindowHandle, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

void Window::setCursorPosition(const std::pair<int, int> &position) {
  auto point = POINT{ .x = mCursorX = position.first, .y = mCursorY = position.second };
  ClientToScreen(mWindowHandle, &point);
}

void Window::createWindow(const WindowInfo &info) {
  mStyle   = WS_OVERLAPPEDWINDOW;
  mExStyle = WS_EX_OVERLAPPEDWINDOW;

  auto rect = RECT{ 0, 0, info.width, info.height };
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
}

LPWSTR Window::lpcstrToLpwstr(LPCSTR cstr) {
  int length = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);

  auto wstr = new WCHAR[length];
  MultiByteToWideChar(CP_UTF8, 0, cstr, -1, wstr, length);
  wstr[length - 1] = L'\0';
  return wstr;
}

void Window::fetchPositionAndSize() {
  auto pos = POINT{};
  ClientToScreen(mWindowHandle, &pos);
  mXPos = pos.x;
  mYPos = pos.y;

  auto area = RECT{};
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
  case WM_MOUSEMOVE: {
    window->mCursorX = GET_X_LPARAM(lParam);
    window->mCursorY = GET_Y_LPARAM(lParam);
    return 0;
  }
  case WM_LBUTTONDOWN: {
    window->mMouseButtons[(size_t)MouseButton::Left] = true;
    return TRUE;
  }
  case WM_RBUTTONDOWN: {
    window->mMouseButtons[(size_t)MouseButton::Right] = true;
    return TRUE;
  }
  case WM_MBUTTONDOWN: {
    window->mMouseButtons[(size_t)MouseButton::Middle] = true;
    return TRUE;
  }
  case WM_XBUTTONDOWN: {
    window->mMouseButtons[(size_t)((lParam == 2) ? MouseButton::X2 : MouseButton::X1)] = true;
    return TRUE;
  }
  case WM_LBUTTONUP: {
    window->mMouseButtons[(size_t)MouseButton::Left] = false;
    return TRUE;
  }
  case WM_RBUTTONUP: {
    window->mMouseButtons[(size_t)MouseButton::Right] = false;
    return TRUE;
  }
  case WM_MBUTTONUP: {
    window->mMouseButtons[(size_t)MouseButton::Middle] = false;
    return TRUE;
  }
  case WM_XBUTTONUP: {
    window->mMouseButtons[(size_t)((lParam == 2) ? MouseButton::X2 : MouseButton::X1)] = false;
    return TRUE;
  }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP: { // Based off of GLFW's code (https://github.com/glfw/glfw/blob/4c64184455f393b85ca55408ece3e8c596af7050/src/win32_window.c#L704)
    WORD keyFlags = HIWORD(lParam);
    BOOL extended = (keyFlags & KF_EXTENDED) == KF_EXTENDED;
    BOOL released = (keyFlags & KF_UP) == KF_UP;
    WORD scanCode = keyFlags & (0xFF | KF_EXTENDED);
    if (!scanCode) {
      scanCode = MapVirtualKeyW(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC);
    }

    if (scanCode == 0x54) scanCode = 0x137;
    if (scanCode == 0x146) scanCode = 0x45;
    if (scanCode == 0x136) scanCode = 0x36;

    KeyCode keyCode = window->mContext->getKeyCode(scanCode);
    if (wParam == VK_CONTROL) {
      if (extended) {
        keyCode = KeyCode::RightControl;
      } else {
        auto        next = MSG{};
        const DWORD time = GetMessageTime();

        if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE)) {
          if (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP ||
              next.message == WM_SYSKEYUP) {
            if (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) && next.time == time) {
              break;
            }
          }
        }
      }

      keyCode = KeyCode::LeftControl;
    } else if (wParam == VK_PROCESSKEY)
      break;

    if (wParam == VK_SNAPSHOT) {
      // Press and release
    } else {
      window->mKeys[(size_t)keyCode] = !released;
    }

    return 0;
  }
  default: break;
  }

  return DefWindowProcW(windowHandle, msg, wParam, lParam);
}

#ifdef _PURRR_BACKEND_VULKAN
VkResult Window::createSurface(VkInstance instance, VkSurfaceKHR *surface) {
  auto createInfo = VkWin32SurfaceCreateInfoKHR{ .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                                 .pNext     = VK_NULL_HANDLE,
                                                 .flags     = 0,
                                                 .hinstance = mContext->getInstance(),
                                                 .hwnd      = mWindowHandle };

  return vkCreateWin32SurfaceKHR(instance, &createInfo, VK_NULL_HANDLE, surface);
}
#endif

} // namespace purrr::win32