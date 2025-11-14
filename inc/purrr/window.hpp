#ifndef _PURRR_WINDOW_HPP_
#define _PURRR_WINDOW_HPP_

#include "purrr/renderTarget.hpp" // IWYU pragma: private
#include "purrr/event.hpp"        // IWYU pragma: private

#include <utility>
#include <bitset>
#include <functional>

namespace purrr {

static constexpr int LENGTH_CSTR = -1;

struct WindowInfo {
  int         width       = -1;
  int         height      = -1;
  const char *title       = "purrr window";
  int         titleLength = LENGTH_CSTR;
  int         xPos        = -1;
  int         yPos        = -1;
};

class Window : public RenderTarget {
private:
  using EventCallback = std::function<void(Window *, const Event *)>;
public:
  Window()          = default;
  virtual ~Window() = default;
public:
  Window(const Window &)            = delete;
  Window &operator=(const Window &) = delete;
public:
  void registerCallback(EventCallback callback) { mCallbacks.push_back(callback); }
public:
  virtual std::pair<int, int> getPosition() const = 0;
public:
  virtual int getTitle(char *title, int length) const = 0;
public:
  std::pair<int, int> getCursorPosition() const { return std::make_pair(mCursorX, mCursorY); }
public:
  bool isMouseButtonDown(MouseButton btn) const { return mMouseButtons[(size_t)btn]; }
  bool isMouseButtonUp(MouseButton btn) const { return !mMouseButtons[(size_t)btn]; }
  bool isKeyDown(KeyCode keyCode) const { return mKeys[(size_t)keyCode]; }
  bool isKeyUp(KeyCode keyCode) const { return !mKeys[(size_t)keyCode]; }
  bool shouldClose() const { return mShouldClose; }
  void shouldClose(bool shouldClose) { mShouldClose = shouldClose; }
public:
  virtual void setSize(const std::pair<int, int> &size)          = 0;
  virtual void setPosition(const std::pair<int, int> &position)  = 0;
  virtual void setTitle(const char *title, int titleLength = -1) = 0;
private:
  double                                  mCursorX      = 0.0f;
  double                                  mCursorY      = 0.0f;
  std::bitset<(size_t)MouseButton::Count> mMouseButtons = {};
  std::bitset<(size_t)KeyCode::Count>     mKeys         = {};
  bool                                    mShouldClose  = false;
private:
  std::vector<EventCallback> mCallbacks = {};
protected:
  void inputWindowMove(int nx, int ny) { announceEvent(new events::WindowMoveEvent(nx, ny)); }

  void inputWindowResize(int nw, int nh) { announceEvent(new events::WindowResizeEvent(nw, nh)); }

  void inputMouseMove(double nx, double ny) {
    mCursorX = nx;
    mCursorY = ny;

    announceEvent(new events::MouseMoveEvent(nx, ny));
  }

  void inputMouseButton(MouseButton btn, bool down) {
    mMouseButtons[(size_t)btn] = down;

    announceEvent(new events::MouseButtonEvent(btn, down));
  }

  void inputMouseWheel(double delta) { announceEvent(new events::MouseWheelEvent(delta)); }

  void inputKey(KeyCode keyCode, bool down) {
    KeyAction action = down ? KeyAction::Press : KeyAction::Release;
    if (down && mKeys[(size_t)keyCode]) action = KeyAction::Repeat;
    mKeys[(size_t)keyCode] = down;

    announceEvent(new events::KeyEvent(keyCode, action));
  }

  void inputShouldClose() { mShouldClose = true; }
private:
  void announceEvent(Event *ev) {
    for (const EventCallback &callback : mCallbacks) {
      callback(this, ev);
    }
  }
};

} // namespace purrr

#endif // _PURRR_WINDOW_HPP_