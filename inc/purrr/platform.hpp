#ifndef _PURRR_PLATFORM_HPP_
#define _PURRR_PLATFORM_HPP_

#include "purrr/win32/context.hpp"

namespace purrr {

#if _PURRR_PLATFORM == win32
namespace platform = win32;
#else
#error "No platform selected!"
#endif

} // namespace purrr

#endif // _PURRR_PLATFORM_HPP_