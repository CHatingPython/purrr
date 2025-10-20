#ifndef _PURRR_IMAGE_HPP_
#define _PURRR_IMAGE_HPP_

#include "purrr/format.hpp"
#include "purrr/object.hpp"

namespace purrr {

enum class ImageTiling {
  Linear,
  Optimal
};

struct ImageInfo {
  size_t      width, height;
  Format      format;
  ImageTiling tiling;
  struct {
    uint8_t texture : 1;
    uint8_t renderTarget : 1;
  } usage;
};

class Image : public Object {
public:
  Image()          = default;
  virtual ~Image() = default;
public:
  Image(const Image &)            = delete;
  Image &operator=(const Image &) = delete;
public:
  virtual void copyData(size_t width, size_t height, size_t size, const void *data) = 0;
};

} // namespace purrr

#endif // _PURRR_IMAGE_HPP_