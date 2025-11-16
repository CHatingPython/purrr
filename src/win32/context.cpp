#include "purrr/win32/context.hpp"
#include "purrr/win32/window.hpp"

#include <cassert>

namespace purrr::win32 {

Context::Context(const ContextInfo &info) {
  (void)info;
  GetModuleHandleExW(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCWSTR>(this),
      &mInstance);

  registerClass();

  assert(QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&mTimerFrequency)));

  fillKeyCodeTable();
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

void Context::fillKeyCodeTable() {
  memset(mKeyCodes, -1, sizeof(mKeyCodes));

  // Taken from GLFW (https://github.com/glfw/glfw/blob/8e15281d34a8b9ee9271ccce38177a3d812456f8/src/win32_init.c#L201)
  mKeyCodes[0x00B] = KeyCode::N0;
  mKeyCodes[0x002] = KeyCode::N1;
  mKeyCodes[0x003] = KeyCode::N2;
  mKeyCodes[0x004] = KeyCode::N3;
  mKeyCodes[0x005] = KeyCode::N4;
  mKeyCodes[0x006] = KeyCode::N5;
  mKeyCodes[0x007] = KeyCode::N6;
  mKeyCodes[0x008] = KeyCode::N7;
  mKeyCodes[0x009] = KeyCode::N8;
  mKeyCodes[0x00A] = KeyCode::N9;
  mKeyCodes[0x01E] = KeyCode::A;
  mKeyCodes[0x030] = KeyCode::B;
  mKeyCodes[0x02E] = KeyCode::C;
  mKeyCodes[0x020] = KeyCode::D;
  mKeyCodes[0x012] = KeyCode::E;
  mKeyCodes[0x021] = KeyCode::F;
  mKeyCodes[0x022] = KeyCode::G;
  mKeyCodes[0x023] = KeyCode::H;
  mKeyCodes[0x017] = KeyCode::I;
  mKeyCodes[0x024] = KeyCode::J;
  mKeyCodes[0x025] = KeyCode::K;
  mKeyCodes[0x026] = KeyCode::L;
  mKeyCodes[0x032] = KeyCode::M;
  mKeyCodes[0x031] = KeyCode::N;
  mKeyCodes[0x018] = KeyCode::O;
  mKeyCodes[0x019] = KeyCode::P;
  mKeyCodes[0x010] = KeyCode::Q;
  mKeyCodes[0x013] = KeyCode::R;
  mKeyCodes[0x01F] = KeyCode::S;
  mKeyCodes[0x014] = KeyCode::T;
  mKeyCodes[0x016] = KeyCode::U;
  mKeyCodes[0x02F] = KeyCode::V;
  mKeyCodes[0x011] = KeyCode::W;
  mKeyCodes[0x02D] = KeyCode::X;
  mKeyCodes[0x015] = KeyCode::Y;
  mKeyCodes[0x02C] = KeyCode::Z;
  mKeyCodes[0x028] = KeyCode::Apostrophe;
  mKeyCodes[0x02B] = KeyCode::Backslash;
  mKeyCodes[0x033] = KeyCode::Comma;
  mKeyCodes[0x00D] = KeyCode::Equal;
  mKeyCodes[0x029] = KeyCode::GraveAccent;
  mKeyCodes[0x01A] = KeyCode::LeftBracket;
  mKeyCodes[0x00C] = KeyCode::Minus;
  mKeyCodes[0x034] = KeyCode::Period;
  mKeyCodes[0x01B] = KeyCode::RightBracket;
  mKeyCodes[0x027] = KeyCode::Semicolon;
  mKeyCodes[0x035] = KeyCode::Slash;
  mKeyCodes[0x056] = KeyCode::World2;
  mKeyCodes[0x00E] = KeyCode::Backspace;
  mKeyCodes[0x153] = KeyCode::Delete;
  mKeyCodes[0x14F] = KeyCode::End;
  mKeyCodes[0x01C] = KeyCode::Enter;
  mKeyCodes[0x001] = KeyCode::Escape;
  mKeyCodes[0x147] = KeyCode::Home;
  mKeyCodes[0x152] = KeyCode::Insert;
  mKeyCodes[0x15D] = KeyCode::Menu;
  mKeyCodes[0x151] = KeyCode::PageDown;
  mKeyCodes[0x149] = KeyCode::PageUp;
  mKeyCodes[0x045] = KeyCode::Pause;
  mKeyCodes[0x039] = KeyCode::Space;
  mKeyCodes[0x00F] = KeyCode::Tab;
  mKeyCodes[0x03A] = KeyCode::CapsLock;
  mKeyCodes[0x145] = KeyCode::NumLock;
  mKeyCodes[0x046] = KeyCode::ScrollLock;
  mKeyCodes[0x03B] = KeyCode::F1;
  mKeyCodes[0x03C] = KeyCode::F2;
  mKeyCodes[0x03D] = KeyCode::F3;
  mKeyCodes[0x03E] = KeyCode::F4;
  mKeyCodes[0x03F] = KeyCode::F5;
  mKeyCodes[0x040] = KeyCode::F6;
  mKeyCodes[0x041] = KeyCode::F7;
  mKeyCodes[0x042] = KeyCode::F8;
  mKeyCodes[0x043] = KeyCode::F9;
  mKeyCodes[0x044] = KeyCode::F10;
  mKeyCodes[0x057] = KeyCode::F11;
  mKeyCodes[0x058] = KeyCode::F12;
  mKeyCodes[0x064] = KeyCode::F13;
  mKeyCodes[0x065] = KeyCode::F14;
  mKeyCodes[0x066] = KeyCode::F15;
  mKeyCodes[0x067] = KeyCode::F16;
  mKeyCodes[0x068] = KeyCode::F17;
  mKeyCodes[0x069] = KeyCode::F18;
  mKeyCodes[0x06A] = KeyCode::F19;
  mKeyCodes[0x06B] = KeyCode::F20;
  mKeyCodes[0x06C] = KeyCode::F21;
  mKeyCodes[0x06D] = KeyCode::F22;
  mKeyCodes[0x06E] = KeyCode::F23;
  mKeyCodes[0x076] = KeyCode::F24;
  mKeyCodes[0x038] = KeyCode::LeftAlt;
  mKeyCodes[0x01D] = KeyCode::LeftControl;
  mKeyCodes[0x02A] = KeyCode::LeftShift;
  mKeyCodes[0x15B] = KeyCode::LeftSuper;
  mKeyCodes[0x137] = KeyCode::PrintScreen;
  mKeyCodes[0x138] = KeyCode::RightAlt;
  mKeyCodes[0x11D] = KeyCode::RightControl;
  mKeyCodes[0x036] = KeyCode::RightShift;
  mKeyCodes[0x15C] = KeyCode::RightSuper;
  mKeyCodes[0x150] = KeyCode::Down;
  mKeyCodes[0x14B] = KeyCode::Left;
  mKeyCodes[0x14D] = KeyCode::Right;
  mKeyCodes[0x148] = KeyCode::Up;
  mKeyCodes[0x052] = KeyCode::Kp0;
  mKeyCodes[0x04F] = KeyCode::Kp1;
  mKeyCodes[0x050] = KeyCode::Kp2;
  mKeyCodes[0x051] = KeyCode::Kp3;
  mKeyCodes[0x04B] = KeyCode::Kp4;
  mKeyCodes[0x04C] = KeyCode::Kp5;
  mKeyCodes[0x04D] = KeyCode::Kp6;
  mKeyCodes[0x047] = KeyCode::Kp7;
  mKeyCodes[0x048] = KeyCode::Kp8;
  mKeyCodes[0x049] = KeyCode::Kp9;
  mKeyCodes[0x04E] = KeyCode::KpAdd;
  mKeyCodes[0x053] = KeyCode::KpDecimal;
  mKeyCodes[0x135] = KeyCode::KpDivide;
  mKeyCodes[0x11C] = KeyCode::KpEnter;
  mKeyCodes[0x059] = KeyCode::KpEqual;
  mKeyCodes[0x037] = KeyCode::KpMultiply;
  mKeyCodes[0x04A] = KeyCode::KpSubtract;
}

void Context::appendRequiredVulkanExtensions(std::vector<const char *> &extensions) {
  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
}

} // namespace purrr::win32