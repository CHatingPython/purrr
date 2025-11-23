#ifdef _PURRR_PLATFORM_X11

#include "purrr/x11/context.hpp"
#include "purrr/x11/window.hpp"

#include <cassert>
#include <cstring>

#include <X11/XKBlib.h>

namespace purrr::platform {
inline namespace x11 {

  Context::Context(const ContextInfo &info) {
    (void)info;
    mDisplay = ::XOpenDisplay(nullptr);
    mContext = XUniqueContext();
    fillKeyCodeTable();

    aWmProtocols  = ::XInternAtom(mDisplay, "WM_PROTOCOLS", False);
    aDeleteWindow = ::XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
    aNetWmName    = ::XInternAtom(mDisplay, "_NET_WM_NAME", False);
    aUtf8String   = ::XInternAtom(mDisplay, "UTF8_STRING", False);
  }

  Context::~Context() {
    ::XCloseDisplay(mDisplay);
  }

  void Context::pollWindowEvents() const {
    ::XEvent event = {};
    while (::XPending(mDisplay) > 0) {
      ::XNextEvent(mDisplay, &event);
      handleEvent(event);
    }

    ::XFlush(mDisplay);
  }

  void Context::waitForWindowEvents() const {
    ::XEvent event = {};
    do {
      ::XNextEvent(mDisplay, &event);
      handleEvent(event);
    } while (::XPending(mDisplay) > 0);

    ::XFlush(mDisplay);
  }

  double Context::getTime() const {
    return 0.0f;
  }

  void Context::fillKeyCodeTable() {
    XkbDescPtr desc = ::XkbGetMap(mDisplay, 0, XkbUseCoreKbd);
    ::XkbGetNames(mDisplay, XkbKeyNamesMask | XkbKeyAliasesMask, desc);

    std::pair<KeyCode, const char *> keymap[] = { { KeyCode::GraveAccent, "TLDE" },
                                                  { KeyCode::N1, "AE01" },
                                                  { KeyCode::N2, "AE02" },
                                                  { KeyCode::N3, "AE03" },
                                                  { KeyCode::N4, "AE04" },
                                                  { KeyCode::N5, "AE05" },
                                                  { KeyCode::N6, "AE06" },
                                                  { KeyCode::N7, "AE07" },
                                                  { KeyCode::N8, "AE08" },
                                                  { KeyCode::N9, "AE09" },
                                                  { KeyCode::N0, "AE10" },
                                                  { KeyCode::Minus, "AE11" },
                                                  { KeyCode::Equal, "AE12" },
                                                  { KeyCode::Q, "AD01" },
                                                  { KeyCode::W, "AD02" },
                                                  { KeyCode::E, "AD03" },
                                                  { KeyCode::R, "AD04" },
                                                  { KeyCode::T, "AD05" },
                                                  { KeyCode::Y, "AD06" },
                                                  { KeyCode::U, "AD07" },
                                                  { KeyCode::I, "AD08" },
                                                  { KeyCode::O, "AD09" },
                                                  { KeyCode::P, "AD10" },
                                                  { KeyCode::LeftBracket, "AD11" },
                                                  { KeyCode::RightBracket, "AD12" },
                                                  { KeyCode::A, "AC01" },
                                                  { KeyCode::S, "AC02" },
                                                  { KeyCode::D, "AC03" },
                                                  { KeyCode::F, "AC04" },
                                                  { KeyCode::G, "AC05" },
                                                  { KeyCode::H, "AC06" },
                                                  { KeyCode::J, "AC07" },
                                                  { KeyCode::K, "AC08" },
                                                  { KeyCode::L, "AC09" },
                                                  { KeyCode::Semicolon, "AC10" },
                                                  { KeyCode::Apostrophe, "AC11" },
                                                  { KeyCode::Z, "AB01" },
                                                  { KeyCode::X, "AB02" },
                                                  { KeyCode::C, "AB03" },
                                                  { KeyCode::V, "AB04" },
                                                  { KeyCode::B, "AB05" },
                                                  { KeyCode::N, "AB06" },
                                                  { KeyCode::M, "AB07" },
                                                  { KeyCode::Comma, "AB08" },
                                                  { KeyCode::Period, "AB09" },
                                                  { KeyCode::Slash, "AB10" },
                                                  { KeyCode::Backslash, "BKSL" },
                                                  { KeyCode::World1, "LSGT" },
                                                  { KeyCode::Space, "SPCE" },
                                                  { KeyCode::Escape, "ESC" },
                                                  { KeyCode::Enter, "RTRN" },
                                                  { KeyCode::Tab, "TAB" },
                                                  { KeyCode::Backspace, "BKSP" },
                                                  { KeyCode::Insert, "INS" },
                                                  { KeyCode::Delete, "DELE" },
                                                  { KeyCode::Right, "RGHT" },
                                                  { KeyCode::Left, "LEFT" },
                                                  { KeyCode::Down, "DOWN" },
                                                  { KeyCode::Up, "UP" },
                                                  { KeyCode::PageUp, "PGUP" },
                                                  { KeyCode::PageDown, "PGDN" },
                                                  { KeyCode::Home, "HOME" },
                                                  { KeyCode::End, "END" },
                                                  { KeyCode::CapsLock, "CAPS" },
                                                  { KeyCode::ScrollLock, "SCLK" },
                                                  { KeyCode::NumLock, "NMLK" },
                                                  { KeyCode::PrintScreen, "PRSC" },
                                                  { KeyCode::Pause, "PAUS" },
                                                  { KeyCode::F1, "FK01" },
                                                  { KeyCode::F2, "FK02" },
                                                  { KeyCode::F3, "FK03" },
                                                  { KeyCode::F4, "FK04" },
                                                  { KeyCode::F5, "FK05" },
                                                  { KeyCode::F6, "FK06" },
                                                  { KeyCode::F7, "FK07" },
                                                  { KeyCode::F8, "FK08" },
                                                  { KeyCode::F9, "FK09" },
                                                  { KeyCode::F10, "FK10" },
                                                  { KeyCode::F11, "FK11" },
                                                  { KeyCode::F12, "FK12" },
                                                  { KeyCode::F13, "FK13" },
                                                  { KeyCode::F14, "FK14" },
                                                  { KeyCode::F15, "FK15" },
                                                  { KeyCode::F16, "FK16" },
                                                  { KeyCode::F17, "FK17" },
                                                  { KeyCode::F18, "FK18" },
                                                  { KeyCode::F19, "FK19" },
                                                  { KeyCode::F20, "FK20" },
                                                  { KeyCode::F21, "FK21" },
                                                  { KeyCode::F22, "FK22" },
                                                  { KeyCode::F23, "FK23" },
                                                  { KeyCode::F24, "FK24" },
                                                  { KeyCode::F25, "FK25" },
                                                  { KeyCode::Kp0, "KP0" },
                                                  { KeyCode::Kp1, "KP1" },
                                                  { KeyCode::Kp2, "KP2" },
                                                  { KeyCode::Kp3, "KP3" },
                                                  { KeyCode::Kp4, "KP4" },
                                                  { KeyCode::Kp5, "KP5" },
                                                  { KeyCode::Kp6, "KP6" },
                                                  { KeyCode::Kp7, "KP7" },
                                                  { KeyCode::Kp8, "KP8" },
                                                  { KeyCode::Kp9, "KP9" },
                                                  { KeyCode::KpDecimal, "KPDL" },
                                                  { KeyCode::KpDivide, "KPDV" },
                                                  { KeyCode::KpMultiply, "KPMU" },
                                                  { KeyCode::KpSubtract, "KPSU" },
                                                  { KeyCode::KpAdd, "KPAD" },
                                                  { KeyCode::KpEnter, "KPEN" },
                                                  { KeyCode::KpEqual, "KPEQ" },
                                                  { KeyCode::LeftShift, "LFSH" },
                                                  { KeyCode::LeftControl, "LCTL" },
                                                  { KeyCode::LeftAlt, "LALT" },
                                                  { KeyCode::LeftSuper, "LWIN" },
                                                  { KeyCode::RightShift, "RTSH" },
                                                  { KeyCode::RightControl, "RCTL" },
                                                  { KeyCode::RightAlt, "RALT" },
                                                  { KeyCode::RightAlt, "LVL3" },
                                                  { KeyCode::RightAlt, "MDSW" },
                                                  { KeyCode::RightSuper, "RWIN" },
                                                  { KeyCode::Menu, "MENU" } };

    for (uint16_t scancode = desc->min_key_code; scancode <= desc->max_key_code; ++scancode) {
      KeyCode key = KeyCode::Count;

      for (size_t i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
        if (std::strncmp(desc->names->keys[scancode].name, keymap[i].second, XkbKeyNameLength) == 0) {
          key = keymap[i].first;
          break;
        }
      }

      for (int i = 0; i < desc->names->num_key_aliases; i++) {
        if (key != KeyCode::Count) break;

        if (std::strncmp(desc->names->key_aliases[i].real, desc->names->keys[scancode].name, XkbKeyNameLength) != 0)
          continue;

        for (size_t j = 0; j < sizeof(keymap) / sizeof(keymap[0]); j++) {
          if (std::strncmp(desc->names->key_aliases[i].alias, keymap[j].second, XkbKeyNameLength) == 0) {
            key = keymap[j].first;
            break;
          }
        }
      }

      mKeyCodes[scancode] = key;
    }

    ::XkbFreeNames(desc, XkbKeyNamesMask, True);
    ::XkbFreeKeyboard(desc, 0, True);
  }

  void Context::handleEvent(::XEvent event) const {
    Window *window = nullptr;
    if (XFindContext(mDisplay, event.xany.window, mContext, reinterpret_cast<XPointer *>(&window)) != 0) return;

    const uint32_t keycode = (event.type == KeyPress || event.type == KeyRelease) ? event.xkey.keycode : 0;

    ::XFilterEvent(&event, None);

    switch (event.type) {
    case ConfigureNotify: {
      if (event.xconfigure.width != window->mWidth || event.xconfigure.height != window->mHeight) {
        window->inputWindowResize(window->mWidth = event.xconfigure.width, window->mHeight = event.xconfigure.height);
      }

      int xPos = event.xconfigure.x;
      int yPos = event.xconfigure.y;
      if (xPos != window->mXPos || yPos != window->mYPos) {
        window->inputWindowMove(window->mXPos = xPos, window->mYPos = yPos);
      }
    } break;
    case ClientMessage: {
      if (event.xclient.message_type == aWmProtocols) {
        const Atom protocol = static_cast<Atom>(event.xclient.data.l[0]);
        if (!protocol) return;

        if (protocol == aDeleteWindow) {
          window->inputShouldClose();
        }
      }
    } break;
    case KeyPress: {
      const KeyCode key = mKeyCodes[keycode];
      window->inputKey(key, true);
    } break;
    case KeyRelease: {
      const KeyCode key = mKeyCodes[keycode];

      if (::XEventsQueued(mDisplay, QueuedAfterReading)) {
        ::XEvent next = {};
        ::XPeekEvent(mDisplay, &next);

        if (next.type == KeyPress && next.xkey.window == event.xkey.window && next.xkey.keycode == keycode &&
            (next.xkey.time - event.xkey.time) < 20)
          break;
      }

      window->inputKey(key, false);
    } break;
    case ButtonPress: {
      switch (event.xbutton.button) {
      case Button1: {
        window->inputMouseButton(MouseButton::Left, true);
      } break;
      case Button2: {
        window->inputMouseButton(MouseButton::Middle, true);
      } break;
      case Button3: {
        window->inputMouseButton(MouseButton::Right, true);
      } break;
      case Button4: {
        window->inputMouseWheel(1.0);
      } break;
      case Button5: {
        window->inputMouseWheel(-1.0);
      } break;
      default: {
        if (event.xbutton.button > 7) {
          window->inputMouseButton(static_cast<MouseButton>(event.xbutton.button - 5), true);
        }
      }
      }
    } break;
    case ButtonRelease: {
      switch (event.xbutton.button) {
      case Button1: {
        window->inputMouseButton(MouseButton::Left, false);
      } break;
      case Button2: {
        window->inputMouseButton(MouseButton::Middle, false);
      } break;
      case Button3: {
        window->inputMouseButton(MouseButton::Right, false);
      } break;
      default: {
        if (event.xbutton.button > 7) {
          window->inputMouseButton(static_cast<MouseButton>(event.xbutton.button - 5), false);
        }
      }
      }
    } break;
    case MotionNotify: {
      window->inputMouseMove(static_cast<double>(event.xmotion.x), static_cast<double>(event.xmotion.y));
    } break;
    }
  }

#ifdef _PURRR_BACKEND_VULKAN
  void Context::appendRequiredVulkanExtensions(std::vector<const char *> &extensions) {
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
  }
#endif // _PURRR_BACKEND_VULKAN

} // namespace x11
} // namespace purrr::platform

#endif // _PURRR_PLATFORM_X11