#ifndef _PURRR_VULKAN_EXCEPTIONS_HPP_
#define _PURRR_VULKAN_EXCEPTIONS_HPP_

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>

namespace purrr {
namespace vulkan {

  using namespace std::string_literals;

  class UnexpectedResult : public std::runtime_error {
  public:
    UnexpectedResult(VkResult result)
      : std::runtime_error("Unexpected result: "s + string_VkResult(result)) {}
  };

  void expectResult(VkResult result, VkResult excepted);

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_EXCEPTIONS_HPP_