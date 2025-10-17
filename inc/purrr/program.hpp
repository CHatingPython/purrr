#ifndef _PURRR_PROGRAM_HPP_
#define _PURRR_PROGRAM_HPP_

#include <cstddef>
#include <cstdint>

#include "purrr/format.hpp"
#include "purrr/object.hpp"

namespace purrr {

enum class ShaderType {
  Vertex,
  Fragment
};

struct ShaderInfo {
  ShaderType  type;
  const char *content;
  size_t      contentLength;
};

enum class VertexinputRate {
  Vertex,
  Instance
};

struct VertexAttribute {
  Format   format;
  uint32_t offset;
};

struct VertexInfo {
  uint32_t               stride;
  VertexinputRate        inputRate;
  const VertexAttribute *attributes;
  size_t                 attributeCount;
};

enum class Topology {
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip
};

enum class CullMode {
  Front,
  Back,
  Both
};

enum class FrontFace {
  Clockwise,
  CounterClockwise
};

struct ProgramInfo {
  const ShaderInfo *shaders;
  size_t            shaderCount;
  const VertexInfo *vertexInfos;
  size_t            vertexInfoCount;
  Topology          topology;
  CullMode          cullMode;
  FrontFace         frontFace;
};

class Program : public Object {
public:
  Program()          = default;
  virtual ~Program() = default;
public:
  Program(const Program &)            = delete;
  Program &operator=(const Program &) = delete;
};

} // namespace purrr

#endif // _PURRR_PROGRAM_HPP_