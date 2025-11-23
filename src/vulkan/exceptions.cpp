#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

namespace purrr::vulkan {

void expectResult(const char *where, VkResult result, VkResult excepted) {
  if (result != excepted) throw UnexpectedResult(result, where);
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN