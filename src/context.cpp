#include "purrr/context.hpp"

// Backends
#include "purrr/vulkan/context.hpp"

namespace purrr {

Context *Context::create(Api api, const ContextInfo &info) {
  switch (api) {
  case Api::Vulkan: return new vulkan::Context(info);
  default: return nullptr;
  }
}

} // namespace purrr