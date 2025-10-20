#include "purrr/program.hpp"
#include "purrr/purrr.hpp"
#include "purrr/programInfoBuilder.hpp"

#include <stdexcept>
#include <vector> // IWYU pragma: keep
#include <fstream>

purrr::ContextClearColor HSVtoRGB(float h, float s, float v);

struct Vertex {
  float x, y;
};
static auto sVertices = std::vector<Vertex>({
    Vertex{ 0.0f, -0.5f },
    Vertex{ -0.5f, 0.5f },
    Vertex{ 0.5f, 0.5f },
});

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
      context->createBuffer(purrr::BufferInfo{ purrr::BufferType::Vertex, sizeof(Vertex) * sVertices.size() });
  vertexBuffer->copy(sVertices.data(), 0, sizeof(Vertex) * sVertices.size());

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

  double lastTime = context->getTime();

  static constexpr float SPEED = 0.1f;

  float hue = 0.0f;

  while (!window->shouldClose()) { // Windows passed to `record` MUST NOT be destroyed before `present`
    context->pollWindowEvents();

    double time = context->getTime();

    hue += (time - lastTime) * SPEED;
    if (hue >= 1.0f) hue -= 1.0f;

    context->begin(); // Wait and begin a command buffer

    if (context->record(window, { { { HSVtoRGB(hue, 1.0f, 1.0f) } } })) { // Begin recording
      context->useProgram(program);
      context->useVertexBuffer(vertexBuffer, 0);
      context->draw(3);

      context->end(); // End recording
    }

    context->submit();

    // If every `record` call returned false and every window passed to `record` is minimized, present
    // will wait on window events. This behaviour can be disabled by passing false.
    context->present();

    lastTime = time;
  }

  context->waitIdle();

  delete program;
  delete vertexBuffer;

  delete window;
  delete context;

  return 0;
}

purrr::ContextClearColor HSVtoRGB(float h, float s, float v) {
  h *= 6.0f;

  int   i = (int)h;
  float f = h - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - f * s);
  float t = v * (1.0f - (1.0f - f) * s);

  switch (i % 6) {
  case 0: return (purrr::ContextClearColor){ v, t, p, 1.0f };
  case 1: return (purrr::ContextClearColor){ q, v, p, 1.0f };
  case 2: return (purrr::ContextClearColor){ p, v, t, 1.0f };
  case 3: return (purrr::ContextClearColor){ p, q, v, 1.0f };
  case 4: return (purrr::ContextClearColor){ t, p, v, 1.0f };
  case 5: return (purrr::ContextClearColor){ v, p, q, 1.0f };
  }
  return (purrr::ContextClearColor){ 0.0f, 0.0f, 0.0f, 1.0f };
}