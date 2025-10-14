#include "purrr/purrr.hpp"

#include <vector> // IWYU pragma: keep

int main(void) {
  purrr::Context *context = purrr::Context::create(
      purrr::Api::Vulkan,
      purrr::ContextInfo{ purrr::Version(1, 1, 0), purrr::VERSION, "purrr" });

  purrr::Window *window = context->createWindow(purrr::WindowInfo{ 1920, 1080, "purrr example" });

  while (!window->shouldClose()) {
    context->pollWindowEvents();

    context->begin(); // Wait

    if (context->record(window)) { // Begin recording
      // ...

      context->end(); // End recording
    }

    context->submit();
    context->present();
  }

  context->waitIdle();

  delete window;
  delete context;

  return 0;
}