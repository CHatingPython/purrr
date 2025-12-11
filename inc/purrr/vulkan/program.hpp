#ifdef _PURRR_BACKEND_VULKAN

#ifndef _PURRR_VULKAN_PROGRAM_HPP_
#define _PURRR_VULKAN_PROGRAM_HPP_

#include <vulkan/vulkan.h>

#include "purrr/program.hpp"

namespace purrr {
namespace vulkan {

  VkShaderStageFlagBits vkShaderType(ShaderType type);
  VkVertexInputRate     vkVertexInputRate(VertexInputRate inputRate);
  VkPrimitiveTopology   vkTopology(Topology topology);
  VkCullModeFlagBits    vkCullMode(CullMode cullMode);
  VkFrontFace           vkFrontFace(FrontFace frontFace);

  class Context;
  class Shader : public purrr::Shader {
  public:
    Shader(Context *context, const ShaderInfo &info);
    ~Shader();
  public:
    virtual Api api() const override { return Api::Vulkan; }
  public:
    VkShaderModule        getModule() const { return mModule; }
    VkShaderStageFlagBits getStage() const { return mStage; }
  private:
    Context              *mContext = nullptr;
    VkShaderModule        mModule  = VK_NULL_HANDLE;
    VkShaderStageFlagBits mStage   = {};
  };

  class IRenderTarget;
  class Program : public purrr::Program {
  public:
    Program(IRenderTarget *renderTarget, Context *context, const ProgramInfo &info);
    ~Program();
  public:
    virtual Api api() const override { return Api::Vulkan; }
  public:
    bool sameRenderTarget(IRenderTarget *renderTarget) const { return mRenderTarget == renderTarget; }
  public:
    VkPipelineLayout getLayout() const { return mLayout; }
    VkPipeline       getPipeline() const { return mPipeline; }
  private:
    IRenderTarget   *mRenderTarget = nullptr;
    Context         *mContext      = nullptr;
    VkPipelineLayout mLayout       = VK_NULL_HANDLE;
    VkPipeline       mPipeline     = VK_NULL_HANDLE;
  private:
    void createLayout(const ProgramInfo &info);
    void createPipeline(const ProgramInfo &info);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_PROGRAM_HPP_

#endif // _PURRR_BACKEND_VULKAN