#ifndef __PURRR_UTILS_SET_HPP_
#define __PURRR_UTILS_SET_HPP_

#include <unordered_set>

namespace purrr {
namespace utils {

  template <typename T>
  std::unordered_set<T> operator-(const std::unordered_set<T> &lhs, const std::unordered_set<T> &rhs) {
    std::unordered_set<T> diff = lhs;
    diff.erase(rhs.begin(), rhs.end());
    return diff;
  }

} // namespace utils
} // namespace purrr

#endif // __PURRR_UTILS_SET_HPP_