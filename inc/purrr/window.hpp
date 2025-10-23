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
  virtual bool shouldClose() const                      = 0;
public:
  virtual void setSize(const std::pair<int, int> &size)               = 0;
  virtual void setPosition(const std::pair<int, int> &position)       = 0;
  virtual void setCursorPosition(const std::pair<int, int> &position) = 0;
  virtual void shouldClose(bool shouldClose)                          = 0;
};

} // namespace purrr

#endif // _PURRR_WINDOW_HPP_