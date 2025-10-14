#include "purrr/purrr.hpp"

#include <vector> // IWYU pragma: keep

int main(void) {
  purrr::Context *context = purrr::Context::create(
      purrr::Api::Vulkan,
      purrr::ContextInfo{ purrr::Version(1, 1, 0), purrr::VERSION, "purrr" });

  purrr::Window *window = context->createWindow(purrr::WindowInfo{ 1920, 1080, "purrr example" });

  while (!window->shouldClose()) {
    context->pollWindowEvents();

    context->record(); // Record commands

    context->submit(); // Submit recorded commands
  }

  delete window;
  delete context;

  return 0;
}