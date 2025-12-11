#ifndef _PURRR_PROGRAM_BUILDER_HPP_
#define _PURRR_PROGRAM_BUILDER_HPP_

#include <stdexcept>
#include <vector>

#include "purrr/program.hpp"
#include "purrr/purrr.hpp"

namespace purrr {

class ProgramBuilder {
public:
  ProgramBuilder() = default;

  ~ProgramBuilder() {
    for (Shader *shader : mMyShaders) {
      delete shader;
    }
    mMyShaders.clear();
  }
public:
  ProgramBuilder &addShader(Shader *shader) {
    mShaders.push_back(shader);
    return *this;
  }

  template <typename... Args>
  ProgramBuilder &addShader(Context *context, Args &&...args) {
    Shader *shader = context->createShader(std::forward<Args>(args)...);
    mMyShaders.push_back(shader);
    mShaders.push_back(shader);
    return *this;
  }

  ProgramBuilder &beginVertexInfo(uint32_t stride, VertexInputRate inputRate) {
    mVertexAttribOffsets.push_back(mVertexAttribs.size());
    mVertexInfos.push_back({ stride, inputRate, nullptr, 0 });
    return *this;
  }

  ProgramBuilder &addVertexAttrib(Format format, uint32_t offset) {
    if (mVertexAttribOffsets.empty()) throw std::runtime_error("Call beginVertexInfo before addVertexAttrib");
    mVertexAttribs.push_back({ format, offset });
    return *this;
  }

  ProgramBuilder &clearVertexInfos() {
    mVertexInfos.clear();
    mVertexAttribs.clear();
    mVertexAttribOffsets.clear();
    return *this;
  }

  ProgramBuilder &addSlot(ProgramSlot slot) {
    mSlots.push_back(slot);
    return *this;
  }

  ProgramBuilder &clearSlots() {
    mSlots.clear();
    return *this;
  }

  ProgramBuilder &setTopology(Topology topology) {
    mTopology = topology;
    return *this;
  }

  ProgramBuilder &setCullMode(CullMode cullMode) {
    mCullMode = cullMode;
    return *this;
  }

  ProgramBuilder &setFrontFace(FrontFace frontFace) {
    mFrontFace = frontFace;
    return *this;
  }
public:
  template <
      typename T,
      typename = std::enable_if<
          std::is_same_v<decltype(std::declval<T *>()->createProgram(std::declval<const ProgramInfo &>())), Program *>>>
  Program *build(T *middleman) {
    for (size_t i = 0; i < mVertexInfos.size(); ++i) {
      mVertexInfos[i].attributes     = &mVertexAttribs[mVertexAttribOffsets[i]];
      mVertexInfos[i].attributeCount = (i + 1 < mVertexInfos.size())
                                           ? (mVertexAttribOffsets[i + 1] - mVertexAttribOffsets[i])
                                           : (mVertexAttribs.size() - mVertexAttribOffsets[i]);
    }

    return middleman->createProgram(ProgramInfo{ .shaders         = mShaders.data(),
                                                 .shaderCount     = mShaders.size(),
                                                 .vertexInfos     = mVertexInfos.data(),
                                                 .vertexInfoCount = mVertexInfos.size(),
                                                 .topology        = mTopology,
                                                 .cullMode        = mCullMode,
                                                 .frontFace       = mFrontFace,
                                                 .slots           = mSlots.data(),
                                                 .slotCount       = mSlots.size() });
  }
private:
  std::vector<Shader *>        mMyShaders           = {};
  std::vector<Shader *>        mShaders             = {};
  std::vector<size_t>          mVertexAttribOffsets = {};
  std::vector<VertexAttribute> mVertexAttribs       = {};
  std::vector<VertexInfo>      mVertexInfos         = {};
  std::vector<ProgramSlot>     mSlots               = {};
  Topology                     mTopology            = Topology::PointList;
  CullMode                     mCullMode            = CullMode::Both;
  FrontFace                    mFrontFace           = FrontFace::Clockwise;
};

} // namespace purrr

#endif // _PURRR_PROGRAM_BUILDER_HPP_