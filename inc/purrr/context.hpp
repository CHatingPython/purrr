#ifndef _PURRR_CONTEXT_HPP_
#define _PURRR_CONTEXT_HPP_

#include "purrr/object.hpp" // IWYU pragma: private
#include "purrr/window.hpp" // IWYU pragma: private
#include <vector>

namespace purrr {

struct Version {
  constexpr Version(uint32_t mj = 1, uint32_t mi = 0, uint32_t pa = 0)
    : major(mj), minor(mi), patch(pa) {}

  constexpr operator uint32_t() const { return (major << 22U) | (minor << 12U) | (patch); }

  uint32_t major;
  uint32_t minor;
  uint32_t patch;
};

struct ContextInfo {
  Version     apiVersion    = {};
  Version     engineVersion = {};
  const char *engineName    = nullptr;
  Version     appVersion    = {};
  const char *appName       = nullptr;
  bool        debug         = false;
};

struct ContextClearColor {
  float r, g, b, a;
};

union ContextClearValue {
  ContextClearColor color;
};

struct RecordClear {
  const std::vector<ContextClearValue> &clearValues;
};

class Context : public Object {
public:
  static Context *create(Api api, const ContextInfo &info = {});
public:
  Context()          = default;
  virtual ~Context() = default;
public:
  Context(const Context &)            = delete;
  Context &operator=(const Context &) = delete;
public:
  virtual void   pollWindowEvents() const    = 0;
  virtual void   waitForWindowEvents() const = 0;
  virtual double getTime() const             = 0;
public:
  virtual Window *createWindow(const WindowInfo &info = {}) = 0;
public:
  virtual void begin()                                          = 0;
  virtual bool record(Window *window, const RecordClear &clear) = 0;
  virtual void end()                                            = 0;
  virtual void submit()                                         = 0;
  virtual void present(bool preventSpinning = true)             = 0;
  virtual void waitIdle()                                       = 0;
};

} // namespace purrr

#endif // _PURRR_CONTEXT_HPP_