#ifndef _PURRR_VULKAN_EXCEPTIONS_HPP_
#define _PURRR_VULKAN_EXCEPTIONS_HPP_

#include <unordered_set>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <exception>
#include <stdexcept>

namespace purrr {
namespace vulkan {

  using namespace std::string_literals;

  class UnexpectedResult : public std::runtime_error {
  public:
    UnexpectedResult(VkResult result, const char *where = nullptr)
      : std::runtime_error(
            "Unexpected result "s + string_VkResult(result) + ((where != nullptr) ? "in "s + where : ""s)) {}
  };

  void expectResult(const char *where, VkResult result, VkResult excepted = VK_SUCCESS);

  template <typename T>
  class NotPresent : public std::exception {
  public:
    NotPresent(const char *what, const std::unordered_set<T> &list)
      : mMessage("Following "), mList(list) {
      mMessage += what;
      mMessage += " were absent: ";

      for (auto it = list.begin(); it != list.end(); ++it) {
        if (it != list.begin()) mMessage += ", ";
        mMessage += *it;
      }
    }

    virtual const char *what() const noexcept override { return mMessage.c_str(); }

    const std::unordered_set<T> &getList() const { return mList; }
  private:
    std::string           mMessage;
    std::unordered_set<T> mList;
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_EXCEPTIONS_HPP_