#include "purrr/vulkan/exceptions.hpp"

namespace purrr::vulkan {

void expectResult(VkResult result, VkResult excepted) {
  if (result != excepted) throw UnexpectedResult(result);
}

} // namespace purrr::vulkan