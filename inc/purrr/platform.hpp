#ifndef _PURRR_PLATFORM_HPP_
#define _PURRR_PLATFORM_HPP_

namespace purrr {
namespace platform {

#if defined(_PURRR_PLATFORM_WIN32)
  inline namespace win32 {}
#else
#error "No platform selected!"
#endif

} // namespace platform
} // namespace purrr

#include "purrr/win32/context.hpp" // IWYU pragma: export
#include "purrr/win32/window.hpp"  // IWYU pragma: export

#endif // _PURRR_PLATFORM_HPP_