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
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    VkShaderModule        getModule() const { return mModule; }
    VkShaderStageFlagBits getStage() const { return mStage; }
  private:
    Context              *mContext = nullptr;
    VkShaderModule        mModule  = VK_NULL_HANDLE;
    VkShaderStageFlagBits mStage   = {};
  };

  class Window;
  class Program : public purrr::Program {
  public:
    Program(Window *window, Context *context, const ProgramInfo &info);
    ~Program();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    bool sameWindow(Window *window) const { return mWindow == window; }
  public:
    VkPipelineLayout getLayout() const { return mLayout; }
    VkPipeline       getPipeline() const { return mPipeline; }
  private:
    Window          *mWindow   = nullptr;
    Context         *mContext  = nullptr;
    VkPipelineLayout mLayout   = VK_NULL_HANDLE;
    VkPipeline       mPipeline = VK_NULL_HANDLE;
  private:
    void createLayout(const ProgramInfo &info);
    void createPipeline(const ProgramInfo &info);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_PROGRAM_HPP_