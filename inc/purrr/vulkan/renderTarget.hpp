#ifdef _PURRR_BACKEND_VULKAN

#ifndef _PURRR_VULKAN_RENDER_TARGET_HPP_
#define _PURRR_VULKAN_RENDER_TARGET_HPP_

#include "purrr/renderTarget.hpp"
#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/image.hpp"

#include <vector>

namespace purrr {
namespace vulkan {

  class IRenderTarget : public purrr::RenderTarget {
  public:
    virtual VkRenderPass getRenderPass() const = 0;
  };

  class RenderTarget : public IRenderTarget {
  public:
    RenderTarget(Context *context, const RenderTargetInfo &info);
    ~RenderTarget();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    virtual purrr::Program *createProgram(const ProgramInfo &info) override;
  public:
    virtual std::pair<int, int> getSize() const override { return std::make_pair(mWidth, mHeight); }
  public:
    bool sameContext(Context *context) const { return mContext == context; }
  public:
    virtual VkRenderPass getRenderPass() const override { return mRenderPass; }
    VkFramebuffer        getFramebuffer() const { return mFramebuffer; }
  private:
    uint32_t mWidth = 0, mHeight = 0;
  private:
    Context             *mContext     = nullptr;
    VkRenderPass         mRenderPass  = VK_NULL_HANDLE;
    VkFramebuffer        mFramebuffer = VK_NULL_HANDLE;
    std::vector<Image *> mImages      = {};
  private:
    void createRenderPass(const RenderTargetInfo &info);
    void createFramebuffer(const RenderTargetInfo &info);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_RENDER_TARGET_HPP_

#endif // _PURRR_BACKEND_VULKAN