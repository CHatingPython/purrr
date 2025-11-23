#ifndef _PURRR_PLATFORM_HPP_
#define _PURRR_PLATFORM_HPP_

#include "purrr/win32/context.hpp" // IWYU pragma: export
#include "purrr/win32/window.hpp" // IWYU pragma: export

namespace purrr {

#if defined(_PURRR_PLATFORM_WIN32)
namespace platform = win32;
#else
#error "No platform selected!"
#endif

} // namespace purrr

#endif // _PURRR_PLATFORM_HPP_