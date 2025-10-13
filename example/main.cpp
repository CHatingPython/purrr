#include "purrr/purrr.hpp"

#include <vector> // IWYU pragma: keep

int main(void) {
  purrr::Context *context = purrr::Context::create(purrr::Api::Vulkan, purrr::ContextInfo{ purrr::VERSION, "purrr" });

  delete context;

  return 0;
}