#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

namespace purrr::win32 {

Context::Context(const ContextInfo &info) {
  GetModuleHandleExW(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(this),
      &mInstance);

  registerClass();
}

Context::~Context() {
  UnregisterClassW(MAKEINTATOM(mWindowClass), mInstance);
}

void Context::registerClass() {
  auto windowClass = WNDCLASSEXW{ .cbSize        = sizeof(WNDCLASSEXW),
                                  .style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC,
                                  .lpfnWndProc   = Window::windowProcedure,
                                  .cbClsExtra    = 0,
                                  .cbWndExtra    = 0,
                                  .hInstance     = mInstance,
                                  .hIcon         = nullptr,
                                  .hCursor       = LoadCursorW(mInstance, IDC_ARROW),
                                  .hbrBackground = nullptr,
                                  .lpszMenuName  = nullptr,
                                  .lpszClassName = L"purrr",
                                  .hIconSm       = nullptr };

  mWindowClass = RegisterClassExW(&windowClass);
}

} // namespace purrr::win32