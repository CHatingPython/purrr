#ifndef _PURRR_EVENT_HPP_
#define _PURRR_EVENT_HPP_

#include <cstdint>

namespace purrr {

enum class MouseButton : uint8_t {
  Left,
  Right,
  Middle,
  X1,
  X2,

  Count
};

// Inspired by GLFW
enum class KeyCode : uint16_t {
  Space        = ' ',
  Apostrophe   = '\'',
  Comma        = ',',
  Minus        = '-',
  Period       = '.',
  Slash        = '/',
  N0           = '0',
  N1           = '1',
  N2           = '2',
  N3           = '3',
  N4           = '4',
  N5           = '5',
  N6           = '6',
  N7           = '7',
  N8           = '8',
  N9           = '9',
  Semicolon    = ';',
  Equal        = '=',
  A            = 'A',
  B            = 'B',
  C            = 'C',
  D            = 'D',
  E            = 'E',
  F            = 'F',
  G            = 'G',
  H            = 'H',
  I            = 'I',
  J            = 'J',
  K            = 'K',
  L            = 'L',
  M            = 'M',
  N            = 'N',
  O            = 'O',
  P            = 'P',
  Q            = 'Q',
  R            = 'R',
  S            = 'S',
  T            = 'T',
  U            = 'U',
  V            = 'V',
  W            = 'W',
  X            = 'X',
  Y            = 'Y',
  Z            = 'Z',
  LeftBracket  = '[',
  Backslash    = '\\',
  RightBracket = ']',
  GraveAccent  = '`',
  World1,
  World2,
  Escape,
  Enter,
  Tab,
  Backspace,
  Insert,
  Delete,
  Right,
  Left,
  Down,
  Up,
  PageUp,
  PageDown,
  Home,
  End,
  CapsLock,
  ScrollLock,
  NumLock,
  PrintScreen,
  Pause,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  F25,
  Kp0,
  Kp1,
  Kp2,
  Kp3,
  Kp4,
  Kp5,
  Kp6,
  Kp7,
  Kp8,
  Kp9,
  KpDecimal,
  KpDivide,
  KpMultiply,
  KpSubtract,
  KpAdd,
  KpEnter,
  KpEqual,
  LeftShift,
  LeftControl,
  LeftAlt,
  LeftSuper,
  RightShift,
  RightControl,
  RightAlt,
  RightSuper,
  Menu,

  Count
};

enum class KeyAction : uint8_t {
  Press,
  Repeat,
  Release
};

enum class EventType {
  WindowMove,
  WindowResize,

  MouseMove,
  MouseButton,
  MouseWheel,

  Key
};

struct Event {
  Event()          = default;
  virtual ~Event() = default;

  virtual constexpr EventType eventType() const = 0;
};

namespace events {

  struct WindowMoveEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::WindowMove; }

    WindowMoveEvent(int x, int y)
      : xPos(x), yPos(y) {}

    int xPos, yPos;
  };

  struct WindowResizeEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::WindowResize; }

    WindowResizeEvent(int w, int h)
      : width(w), height(h) {}

    int width, height;
  };

  struct MouseMoveEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::MouseMove; }

    MouseMoveEvent(double x, double y)
      : xPos(x), yPos(y) {}

    double xPos, yPos;
  };

  struct MouseButtonEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::MouseButton; }

    MouseButtonEvent(MouseButton btn, bool p)
      : button(btn), pressed(p) {}

    MouseButton button;
    bool        pressed;
  };

  struct MouseWheelEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::MouseWheel; }

    MouseWheelEvent(double d)
      : delta(d) {}

    double delta;
  };

  struct KeyEvent : public Event {
    virtual constexpr EventType eventType() const override { return EventType::Key; }

    KeyEvent(KeyCode code, KeyAction ac)
      : keyCode(code), action(ac) {}

    KeyCode   keyCode;
    KeyAction action;
  };

} // namespace events

} // namespace purrr

#endif // _PURRR_EVENT_HPP_