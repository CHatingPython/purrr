#ifndef _PURRR_CONTEXT_HPP_
#define _PURRR_CONTEXT_HPP_

#include "purrr/object.hpp" // IWYU pragma: private

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

class Context : public Object {
public:
  static Context *create(Api api, const ContextInfo &info = {});
public:
  Context()          = default;
  virtual ~Context() = default;
public:
  Context(const Context &)            = delete;
  Context &operator=(const Context &) = delete;
};

} // namespace purrr

#endif // _PURRR_CONTEXT_HPP_