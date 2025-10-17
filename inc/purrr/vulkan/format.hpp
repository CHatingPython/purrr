#ifndef _PURRR_VULKAN_FORMAT_HPP_
#define _PURRR_VULKAN_FORMAT_HPP_

#include <vulkan/vulkan.h>

#include "purrr/format.hpp"

namespace purrr {

VkFormat vkFormat(Format format);
Format   format(VkFormat vkFormat);

} // namespace purrr

#endif // _PURRR_VULKAN_FORMAT_HPP_