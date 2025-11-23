#ifndef _PURRR_SAMPLER_HPP_
#define _PURRR_SAMPLER_HPP_

#include "purrr/object.hpp"

namespace purrr {

enum class Filter {
  Nearest,
  Linear
};

enum class SamplerAddressMode {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder
};

struct SamplerInfo {
  Filter             magFilter;
  Filter             minFilter;
  Filter             mipFilter;
  SamplerAddressMode addressModeU;
  SamplerAddressMode addressModeV;
  SamplerAddressMode addressModeW;
};

class Sampler : public Object {
public:
  Sampler()          = default;
  virtual ~Sampler() = default;
public:
  Sampler(const Sampler &)            = delete;
  Sampler &operator=(const Sampler &) = delete;
};

} // namespace purrr

#endif // _PURRR_SAMPLER_HPP_