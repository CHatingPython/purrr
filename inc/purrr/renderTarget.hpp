#ifndef _PURRR_RENDER_TARGET_HPP_
#define _PURRR_RENDER_TARGET_HPP_

#include "purrr/object.hpp"
#include "purrr/image.hpp"

#include <utility>

namespace purrr {

struct RenderTargetInfo {
  int     width;
  int     height;
  Image **images;
  size_t  imageCount;
};

class RenderTarget : public Object {
public:
  RenderTarget()          = default;
  virtual ~RenderTarget() = default;
public:
  RenderTarget(const RenderTarget &)            = delete;
  RenderTarget &operator=(const RenderTarget &) = delete;
public:
  virtual std::pair<int, int> getSize() const = 0;
};

} // namespace purrr

#endif // _PURRR_RENDER_TARGET_HPP_