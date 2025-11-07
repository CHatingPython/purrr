/*
  Simple paint program
 */

#include "purrr/purrr.hpp"
#include "purrr/programBuilder.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector> // IWYU pragma: keep
#include <fstream>

std::vector<char> readFile(const char *filepath) {
  std::ifstream file(filepath, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
  if (!file.is_open()) throw std::runtime_error("Failed to open a file");
  auto length = file.tellg();

  auto content = std::vector<char>(length);
  file.seekg(0);
  file.read(content.data(), content.size());

  return content;
}

static purrr::Context *sContext = nullptr;
static purrr::Sampler *sSampler = nullptr;

struct Canvas {
  Canvas(size_t width, size_t height)
    : image(sContext->createImage(purrr::ImageInfo{ .width   = width,
                                                    .height  = height,
                                                    .format  = purrr::Format::RGBA8Srgb,
                                                    .tiling  = purrr::ImageTiling::Optimal,
                                                    .usage   = { .texture = true },
                                                    .sampler = sSampler })),
      pixels(new uint8_t[width * height * 4]),
      width(width),
      height(height) {
    memset(pixels, 0xFF, width * height * 4);
    image->copyData(width, height, width * height * 4, pixels);
  }

  ~Canvas() {
    delete[] pixels;
    delete image;

    pixels = nullptr;
    image  = nullptr;
  }

  void draw(size_t x, size_t y) {
    for (int oy = -1; oy <= 1; ++oy) {
      size_t index = (y + oy) * width + x - 1;
      for (int i = 0; i < 3; ++i) {
        pixels[(index * 4) + 0] = 0x00; // R
        pixels[(index * 4) + 1] = 0x00; // G
        pixels[(index * 4) + 2] = 0x00; // B
        pixels[(index * 4) + 3] = 0xFF; // A
        ++index;
      }
    }
    image->copyData(width, height, width * height * 4, pixels);
  }

  purrr::Image *image;
  uint8_t      *pixels;
  size_t        width;
  size_t        height;
};

int main(void) {
  sContext = purrr::Context::create(
      purrr::Api::Vulkan,
      purrr::ContextInfo{ purrr::Version(1, 1, 0), purrr::VERSION, "purrr" });

  sSampler = sContext->createSampler(purrr::SamplerInfo{ .magFilter    = purrr::Filter::Nearest,
                                                         .minFilter    = purrr::Filter::Nearest,
                                                         .mipFilter    = purrr::Filter::Nearest,
                                                         .addressModeU = purrr::SamplerAddressMode::Repeat,
                                                         .addressModeV = purrr::SamplerAddressMode::Repeat,
                                                         .addressModeW = purrr::SamplerAddressMode::Repeat });

  purrr::Window *window = sContext->createWindow(purrr::WindowInfo{ 1920, 1080, "simple paint" });

  purrr::Shader *vertexShader   = sContext->createShader(purrr::ShaderType::Vertex, readFile("./shader.vert.spv"));
  purrr::Shader *fragmentShader = sContext->createShader(purrr::ShaderType::Fragment, readFile("./shader.frag.spv"));

  purrr::Program *program = purrr::ProgramBuilder()
                                .addShader(vertexShader)
                                .addShader(fragmentShader)
                                .setCullMode(purrr::CullMode::Back)
                                .setFrontFace(purrr::FrontFace::CounterClockwise)
                                .setTopology(purrr::Topology::TriangleStrip)
                                .addSlot(purrr::ProgramSlot::Texture)
                                .build(window);

  delete vertexShader;
  delete fragmentShader;

  Canvas canvas(window->getSize().first, window->getSize().second);

  while (!window->shouldClose()) { // Windows passed to `record` MUST NOT be destroyed before `present`
    sContext->pollWindowEvents();

    if (window->isMouseButtonDown(purrr::MouseButton::Left)) {
      auto [mx, my] = window->getCursorPosition();
      canvas.draw(mx, my);
    }

    sContext->begin(); // Wait and begin a command buffer

    if (sContext->record(window, { { { 0.0f, 0.0f, 0.0f, 1.0f } } })) { // Begin recording
      sContext->useProgram(program);
      sContext->useTextureImage(canvas.image, 0);
      sContext->draw(4); // Draw a full-screen square

      sContext->end(); // End recording
    }

    sContext->submit();

    // If every `record` call returned false and every window passed to `record` is minimized, present
    // will wait on window events. This behaviour can be disabled by passing false.
    sContext->present();
  }

  sContext->waitIdle();

  canvas.~Canvas();
  delete sSampler;
  delete program;

  delete window;
  delete sContext;

  return 0;
}