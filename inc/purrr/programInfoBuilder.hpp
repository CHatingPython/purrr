#ifndef _PURRR_PROGRAM_INFO_BUILDER_HPP_
#define _PURRR_PROGRAM_INFO_BUILDER_HPP_

#include <stdexcept>
#include <vector>

#include "purrr/program.hpp"

namespace purrr {

class ProgramInfoBuilder {
public:
  ProgramInfoBuilder() = default;
public:
  ProgramInfoBuilder &addShader(Shader *shader) {
    mShaders.push_back(shader);
    return *this;
  }

  ProgramInfoBuilder &beginVertexInfo(uint32_t stride, VertexInputRate inputRate) {
    mVertexAttribOffsets.push_back(mVertexAttribs.size());
    mVertexInfos.emplace_back(stride, inputRate, nullptr, 0);
    return *this;
  }

  ProgramInfoBuilder &addVertexAttrib(Format format, uint32_t offset) {
    if (mVertexAttribOffsets.empty()) throw std::runtime_error("Call beginVertexInfo before addVertexAttrib");
    mVertexAttribs.emplace_back(format, offset);
    return *this;
  }

  ProgramInfoBuilder &setTopology(Topology topology) {
    mTopology = topology;
    return *this;
  }

  ProgramInfoBuilder &setCullMode(CullMode cullMode) {
    mCullMode = cullMode;
    return *this;
  }

  ProgramInfoBuilder &setFrontFace(FrontFace frontFace) {
    mFrontFace = frontFace;
    return *this;
  }
public:
  ProgramInfo build() {
    for (size_t i = 0; i < mVertexInfos.size(); ++i) {
      mVertexInfos[i].attributes     = &mVertexAttribs[mVertexAttribOffsets[i]];
      mVertexInfos[i].attributeCount = (i + 1 < mVertexInfos.size())
                                           ? (mVertexAttribOffsets[i + 1] - mVertexAttribOffsets[i])
                                           : (mVertexAttribs.size() - mVertexAttribOffsets[i]);
    }

    return ProgramInfo{ .shaders         = mShaders.data(),
                        .shaderCount     = mShaders.size(),
                        .vertexInfos     = mVertexInfos.data(),
                        .vertexInfoCount = mVertexInfos.size(),
                        .topology        = mTopology,
                        .cullMode        = mCullMode,
                        .frontFace       = mFrontFace };
  }
private:
  std::vector<Shader *>        mShaders             = {};
  std::vector<size_t>          mVertexAttribOffsets = {};
  std::vector<VertexAttribute> mVertexAttribs       = {};
  std::vector<VertexInfo>      mVertexInfos         = {};
  Topology                     mTopology            = Topology::PointList;
  CullMode                     mCullMode            = CullMode::Both;
  FrontFace                    mFrontFace           = FrontFace::Clockwise;
};

} // namespace purrr

#endif // _PURRR_PROGRAM_INFO_BUILDER_HPP_