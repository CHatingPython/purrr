#if _PURRR_PLATFORM == win32

#ifndef _PURRR_WIN32_CONTEXT_HPP_
#define _PURRR_WIN32_CONTEXT_HPP_

#include "purrr/context.hpp"

#define UNICODE
#define _UNICODE
#include <Windows.h>

#include <utility>

namespace purrr {
namespace win32 {

  class Context : public purrr::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    Context(Context &&other) { *this = std::move(other); }

    Context &operator=(Context &&other) {
      if (this == &other) return *this;

      mInstance    = other.mInstance;
      mWindowClass = other.mWindowClass;

      other.mInstance    = nullptr;
      other.mWindowClass = INVALID_ATOM;

      return *this;
    }
  public:
    HINSTANCE getInstance() const { return mInstance; }
    ATOM      getWindowClass() const { return mWindowClass; }
  private:
    HINSTANCE mInstance    = nullptr;
    ATOM      mWindowClass = INVALID_ATOM;
  private:
    void getInstance();
    void registerClass();
  };

} // namespace win32
} // namespace purrr

#endif // _PURRR_WIN32_CONTEXT_HPP_

#endif // _PURRR_PLATFORM == win32