#ifndef _PURRR_VULKAN_FORMAT_HPP_
#define _PURRR_VULKAN_FORMAT_HPP_

#include <vulkan/vulkan.h>

#include "purrr/format.hpp"

namespace purrr {
namespace vulkan {

  VkFormat vkFormat(Format format);
  Format   format(VkFormat vkFormat);

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_FORMAT_HPP_