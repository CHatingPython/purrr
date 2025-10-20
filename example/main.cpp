#include "purrr/purrr.hpp"
#include "purrr/programInfoBuilder.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>
#include <vector> // IWYU pragma: keep
#include <fstream>

static struct Vertex {
  float x, y;
} sVertices[] = { Vertex{ -1.0f, -1.0f }, Vertex{ -1.0f, 1.0f }, Vertex{ 1.0f, -1.0f }, Vertex{ 1.0f, 1.0f } };

static uint32_t sIndices[] = { 0, 1, 2, 2, 1, 3 };

std::vector<char> readFile(const char *filepath) {
  std::ifstream file(filepath, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
  if (!file.is_open()) throw std::runtime_error("Failed to open a file");
  auto length = file.tellg();

  auto content = std::vector<char>(length);
  file.seekg(0);
  file.read(content.data(), content.size());

  return content;
}

int main(void) {
  purrr::Context *context = purrr::Context::create(
      purrr::Api::Vulkan,
      purrr::ContextInfo{ purrr::Version(1, 1, 0), purrr::VERSION, "purrr" });

  purrr::Window *window = context->createWindow(purrr::WindowInfo{ 1920, 1080, "purrr example" });

  purrr::Buffer *vertexBuffer =
      context->createBuffer(purrr::BufferInfo{ purrr::BufferType::Vertex, sizeof(sVertices) });
  vertexBuffer->copy(sVertices, 0, sizeof(sVertices));

  purrr::Buffer *indexBuffer = context->createBuffer(purrr::BufferInfo{ purrr::BufferType::Index, sizeof(sIndices) });
  indexBuffer->copy(sIndices, 0, sizeof(sIndices));

  purrr::Shader *vertexShader   = context->createShader(purrr::ShaderType::Vertex, readFile("./shader.vert.spv"));
  purrr::Shader *fragmentShader = context->createShader(purrr::ShaderType::Fragment, readFile("./shader.frag.spv"));

  purrr::Program *program = window->createProgram(purrr::ProgramInfoBuilder()
                                                      .addShader(vertexShader)
                                                      .addShader(fragmentShader)
                                                      .beginVertexInfo(sizeof(Vertex), purrr::VertexInputRate::Vertex)
                                                      .addVertexAttrib(purrr::Format::RG32Sfloat, 0) // position
                                                      .setCullMode(purrr::CullMode::Back)
                                                      .setFrontFace(purrr::FrontFace::CounterClockwise)
                                                      .setTopology(purrr::Topology::TriangleList)
                                                      .build());

  delete vertexShader;
  delete fragmentShader;

  purrr::Sampler *sampler =
      context->createSampler(purrr::SamplerInfo{ .magFilter    = purrr::Filter::Linear,
                                                 .minFilter    = purrr::Filter::Linear,
                                                 .mipFilter    = purrr::Filter::Linear,
                                                 .addressModeU = purrr::SamplerAddressMode::Repeat,
                                                 .addressModeV = purrr::SamplerAddressMode::Repeat,
                                                 .addressModeW = purrr::SamplerAddressMode::Repeat });

  purrr::Image *image = nullptr;

  {
    int      width = 0, height = 0;
    stbi_uc *pixels = stbi_load("./image.png", &width, &height, nullptr, STBI_rgb_alpha);

    image = context->createImage(purrr::ImageInfo{ .width  = static_cast<size_t>(width),
                                                   .height = static_cast<size_t>(height),
                                                   .format = purrr::Format::RGBA8Srgb,
                                                   .tiling = purrr::ImageTiling::Optimal,
                                                   .usage  = { .texture = true } });

    image->copyData(width, height, width * height * 4, pixels);

    stbi_image_free(pixels);
  }

  double lastTime = context->getTime();

  static constexpr float SPEED = 0.1f;

  while (!window->shouldClose()) { // Windows passed to `record` MUST NOT be destroyed before `present`
    context->pollWindowEvents();

    double time = context->getTime();

    context->begin(); // Wait and begin a command buffer

    if (context->record(window, { { { 0.0f, 0.0f, 0.0f, 1.0f } } })) { // Begin recording
      context->useProgram(program);
      context->useVertexBuffer(vertexBuffer, 0);
      context->useIndexBuffer(indexBuffer, purrr::IndexType::U32);
      context->drawIndexed(6); // Draw a full-screen square

      context->end(); // End recording
    }

    context->submit();

    // If every `record` call returned false and every window passed to `record` is minimized, present
    // will wait on window events. This behaviour can be disabled by passing false.
    context->present();

    lastTime = time;
  }

  context->waitIdle();

  delete image;
  delete sampler;
  delete program;
  delete indexBuffer;
  delete vertexBuffer;

  delete window;
  delete context;

  return 0;
}