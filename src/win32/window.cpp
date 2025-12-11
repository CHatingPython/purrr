#ifdef _PURRR_PLATFORM_WIN32

#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

#include <stdexcept>
#include <cassert>

// For GET_X_LPARAM I think
#include <windowsx.h> // IWYU pragma: keep

#undef min

namespace purrr::platform {
inline namespace win32 {

  Window::Window(Context *context, const WindowInfo &info)
    : mContext(context) {
    createWindow(info);
    fetchPositionAndSize();

    {
      POINT position{};
      GetCursorPos(&position);
      inputMouseMove(position.x, position.y);
    }
  }

  Window::~Window() {
    if (mWindowHandle) DestroyWindow(mWindowHandle);
  }

  int Window::getTitle(char *title, int length) const {
    assert(length > 0);
    LPWSTR wideTitle  = new WCHAR[length + 1];
    int    wideLength = GetWindowTextW(mWindowHandle, wideTitle, length + 1);

    LPSTR str       = lpwstrToLpcstr(wideTitle, wideLength);
    int   strLength = static_cast<int>(strlen(str));
    int   minLength = std::min(length - 1, strLength);

    memcpy(title, str, minLength);
    title[minLength] = '\0';
    return minLength;
  }

  void Window::setSize(const std::pair<int, int> &size) {
    RECT rect{ 0, 0, size.first, size.second };

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
    RECT rect{ position.first, position.second, 0, 0 };
    AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);
    SetWindowPos(mWindowHandle, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
  }

  void Window::setTitle(const char *title, int titleLength) {
    LPWSTR wideTitle = lpcstrToLpwstr(title, titleLength);
    SetWindowTextW(mWindowHandle, wideTitle);
    delete[] wideTitle;
  }

  void Window::createWindow(const WindowInfo &info) {
    mStyle   = WS_OVERLAPPEDWINDOW;
    mExStyle = WS_EX_OVERLAPPEDWINDOW;

    RECT rect{ 0, 0, info.width, info.height };
    ::AdjustWindowRectEx(&rect, mStyle, FALSE, mExStyle);

    int xPos = (info.xPos < 0) ? CW_USEDEFAULT : (info.xPos + rect.left);
    int yPos = (info.yPos < 0) ? CW_USEDEFAULT : (info.yPos + rect.top);

    int width  = (info.width < 0) ? CW_USEDEFAULT : (rect.right - rect.left);
    int height = (info.height < 0) ? CW_USEDEFAULT : (rect.bottom - rect.top);

    LPWSTR wideTitle = lpcstrToLpwstr(info.title, info.titleLength);

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

  LPWSTR Window::lpcstrToLpwstr(LPCSTR cstr, int length) const {
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, cstr, length, nullptr, 0);

    auto wstr = new WCHAR[wideLength];
    MultiByteToWideChar(CP_UTF8, 0, cstr, length, wstr, wideLength);
    wstr[wideLength - 1] = L'\0';
    return wstr;
  }

  LPSTR Window::lpwstrToLpcstr(LPCWSTR wstr, int wideLength) const {
    int length = WideCharToMultiByte(CP_UTF8, 0, wstr, wideLength, nullptr, 0, nullptr, nullptr);

    auto cstr = new CHAR[length];
    WideCharToMultiByte(CP_UTF8, 0, wstr, wideLength, cstr, length, nullptr, nullptr);
    cstr[length - 1] = '\0';
    return cstr;
  }

  LPSTR Window::strdup(LPCSTR cstr, int length) {
    if (length < 0) length = static_cast<int>(strlen(cstr));
    LPSTR str   = new CHAR[length + 1];
    str[length] = '\0';
    memcpy(str, cstr, length);
    return str;
  }

  void Window::fetchPositionAndSize() {
    POINT pos{};
    ClientToScreen(mWindowHandle, &pos);
    mXPos = static_cast<WORD>(pos.x);
    mYPos = static_cast<WORD>(pos.y);

    RECT area{};
    GetClientRect(mWindowHandle, &area);
    mWidth  = static_cast<WORD>(area.right);
    mHeight = static_cast<WORD>(area.bottom);
  }

  LRESULT Window::windowProcedure(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window *window = reinterpret_cast<Window *>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));
    if (window == nullptr) return DefWindowProcW(windowHandle, msg, wParam, lParam);

    switch (msg) {
    case WM_CLOSE: {
      window->inputShouldClose();
      return 0;
    }
    case WM_MOVE: {
      window->mXPos = LOWORD(lParam);
      window->mYPos = HIWORD(lParam);
      window->inputWindowMove(window->mXPos, window->mYPos);
      return 0;
    }
    case WM_SIZE: {
      window->mWidth  = LOWORD(lParam);
      window->mHeight = HIWORD(lParam);
      window->inputWindowResize(window->mWidth, window->mHeight);
      return 0;
    }
    case WM_MOUSEMOVE: {
      window->inputMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      return 0;
    }
    case WM_LBUTTONDOWN: {
      window->inputMouseButton(MouseButton::Left, true);
      return TRUE;
    }
    case WM_RBUTTONDOWN: {
      window->inputMouseButton(MouseButton::Right, true);
      return TRUE;
    }
    case WM_MBUTTONDOWN: {
      window->inputMouseButton(MouseButton::Middle, true);
      return TRUE;
    }
    case WM_XBUTTONDOWN: {
      window->inputMouseButton(((lParam == 2) ? MouseButton::X2 : MouseButton::X1), true);
      return TRUE;
    }
    case WM_LBUTTONUP: {
      window->inputMouseButton(MouseButton::Left, false);
      return TRUE;
    }
    case WM_RBUTTONUP: {
      window->inputMouseButton(MouseButton::Right, false);
      return TRUE;
    }
    case WM_MBUTTONUP: {
      window->inputMouseButton(MouseButton::Middle, false);
      return TRUE;
    }
    case WM_XBUTTONUP: {
      window->inputMouseButton(((lParam == 2) ? MouseButton::X2 : MouseButton::X1), false);
      return TRUE;
    }
    case WM_MOUSEWHEEL: {
      window->inputMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
      return 0;
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
        scanCode = static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>(wParam), MAPVK_VK_TO_VSC));
      }

      if (scanCode == 0x54) scanCode = 0x137;
      if (scanCode == 0x146) scanCode = 0x45;
      if (scanCode == 0x136) scanCode = 0x36;

      KeyCode keyCode = window->mContext->getKeyCode(scanCode);
      if (wParam == VK_CONTROL) {
        if (extended) {
          keyCode = KeyCode::RightControl;
        } else {
          MSG next{};

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
        window->inputKey(KeyCode::PrintScreen, true);
        window->inputKey(KeyCode::PrintScreen, false);
      } else {
        window->inputKey(keyCode, !released);
      }

      return 0;
    }
    default: break;
    }

    return DefWindowProcW(windowHandle, msg, wParam, lParam);
  }

#ifdef _PURRR_BACKEND_VULKAN
  VkResult Window::createSurface(VkInstance instance, VkSurfaceKHR *surface) {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext     = VK_NULL_HANDLE;
    createInfo.flags     = 0;
    createInfo.hinstance = mContext->getInstance();
    createInfo.hwnd      = mWindowHandle;

    return vkCreateWin32SurfaceKHR(instance, &createInfo, VK_NULL_HANDLE, surface);
  }
#endif

} // namespace win32
} // namespace purrr::platform

#endif // _PURRR_PLATFORM_WIN32