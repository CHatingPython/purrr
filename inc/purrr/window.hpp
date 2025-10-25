#ifndef _PURRR_WINDOW_HPP_
#define _PURRR_WINDOW_HPP_

#include "purrr/renderTarget.hpp" // IWYU pragma: private
#include "purrr/program.hpp"      // IWYU pragma: private
#include "purrr/event.hpp"        // IWYU pragma: private

#include <utility>
#include <bitset>

namespace purrr {

struct WindowInfo {
  int         width       = -1;
  int         height      = -1;
  const char *title       = "purrr window";
  size_t      titleLength = -1;
  int         xPos        = -1;
  int         yPos        = -1;
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
  bool isMouseButtonDown(MouseButton btn) const { return mMouseButtons[(size_t)btn]; }
  bool isMouseButtonUp(MouseButton btn) const { return !mMouseButtons[(size_t)btn]; }
  bool isKeyDown(KeyCode keyCode) const { return mKeys[(size_t)keyCode]; }
  bool isKeyUp(KeyCode keyCode) const { return !mKeys[(size_t)keyCode]; }
  bool shouldClose() const { return mShouldClose; }
  void shouldClose(bool shouldClose) { mShouldClose = shouldClose; }
public:
  virtual void setSize(const std::pair<int, int> &size)               = 0;
  virtual void setPosition(const std::pair<int, int> &position)       = 0;
  virtual void setCursorPosition(const std::pair<int, int> &position) = 0;
private:
  std::bitset<(size_t)MouseButton::Count> mMouseButtons = {};
  std::bitset<(size_t)KeyCode::Count>     mKeys         = {};
  bool                                    mShouldClose  = false;
protected:
  void inputMouseButton(MouseButton btn, bool down) { mMouseButtons[(size_t)btn] = down; }
  void inputKey(KeyCode keyCode, bool down) { mKeys[(size_t)keyCode] = down; }
  void inputShouldClose() { mShouldClose = true; }
};

} // namespace purrr

#endif // _PURRR_WINDOW_HPP_