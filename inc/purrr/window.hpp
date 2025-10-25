#ifndef _PURRR_WINDOW_HPP_
#define _PURRR_WINDOW_HPP_

#include "purrr/renderTarget.hpp" // IWYU pragma: private
#include "purrr/program.hpp"      // IWYU pragma: private

#include <utility>

namespace purrr {

struct WindowInfo {
  int         width       = -1;
  int         height      = -1;
  const char *title       = "purrr window";
  size_t      titleLength = -1;
  int         xPos        = -1;
  int         yPos        = -1;
};

using MouseButton = uint8_t;
struct MouseButtons {
  static constexpr size_t Left   = 0;
  static constexpr size_t Right  = 1;
  static constexpr size_t Middle = 2;
  static constexpr size_t X1     = 3;
  static constexpr size_t X2     = 4;
  static constexpr size_t COUNT  = 5;
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

class Window : public RenderTarget {
public:
  Window()          = default;
  virtual ~Window() = default;
public:
  Window(const Window &)            = delete;
  Window &operator=(const Window &) = delete;
public:
  virtual Program *createProgram(const ProgramInfo &info) = 0;
public:
  virtual std::pair<int, int> getPosition() const       = 0;
  virtual std::pair<int, int> getCursorPosition() const = 0;
public:
  virtual bool isMouseButtonDown(MouseButton btn) const = 0;
  virtual bool isMouseButtonUp(MouseButton btn) const   = 0;
  virtual bool isKeyDown(KeyCode keyCode) const         = 0;
  virtual bool isKeyUp(KeyCode keyCode) const           = 0;
  virtual bool shouldClose() const                      = 0;
public:
  virtual void setSize(const std::pair<int, int> &size)               = 0;
  virtual void setPosition(const std::pair<int, int> &position)       = 0;
  virtual void setCursorPosition(const std::pair<int, int> &position) = 0;
  virtual void shouldClose(bool shouldClose)                          = 0;
};

} // namespace purrr

#endif // _PURRR_WINDOW_HPP_