#ifndef _PURRR_OBJECT_HPP_
#define _PURRR_OBJECT_HPP_

#include <cstdint>

namespace purrr {

enum class Api : uint8_t {
  Vulkan = 0,

  Custom = 255
};

class Object {
public:
  virtual Api api() const = 0;
};

} // namespace purrr

#endif // _PURRR_OBJECT_HPP_