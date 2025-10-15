#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

#include <cassert>

namespace purrr::win32 {

Context::Context(const ContextInfo &info) {
  GetModuleHandleExW(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(this),
      &mInstance);

  registerClass();

  assert(QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&mTimerFrequency)));
}

Context::~Context() {
  UnregisterClassW(MAKEINTATOM(mWindowClass), mInstance);
}

void Context::pollWindowEvents() const {
  MSG msg = {};
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void Context::waitForWindowEvents() const {
  WaitMessage();
  pollWindowEvents();
}

double Context::getTime() const {
  uint64_t value = 0;
  assert(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&value)));
  return (double)value / mTimerFrequency;
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

void Context::appendRequiredVulkanExtensions(std::vector<const char *> &extensions) {
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

} // namespace purrr::win32