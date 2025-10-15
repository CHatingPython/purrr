#include "purrr/purrr.hpp"

#include <vector> // IWYU pragma: keep

int main(void) {
  purrr::Context *context = purrr::Context::create(
      purrr::Api::Vulkan,
      purrr::ContextInfo{ purrr::Version(1, 1, 0), purrr::VERSION, "purrr" });

  purrr::Window *window = context->createWindow(purrr::WindowInfo{ 1920, 1080, "purrr example" });

  while (!window->shouldClose()) { // Windows passed to `record` MUST NOT be destroyed before `present`
    context->pollWindowEvents();

    context->begin(); // Wait and begin a command buffer

    if (context->record(window, { { { 1.0f, 1.0f, 1.0f, 1.0f } } })) { // Begin recording
      // ...

      context->end(); // End recording
    }

    context->submit();

    // If every `record` call returned false and every window passed to `record` is minimized, present
    // will wait on window events. This behaviour can be disabled by passing false.
    context->present();
  }

  context->waitIdle();

  delete window;
  delete context;

  return 0;
}