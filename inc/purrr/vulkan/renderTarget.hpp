#ifndef _PURRR_VULKAN_RENDER_TARGET_HPP_
#define _PURRR_VULKAN_RENDER_TARGET_HPP_

#include "purrr/renderTarget.hpp"
#include "purrr/vulkan/context.hpp"
#include "purrr/vulkan/image.hpp"

#include <vector>

namespace purrr {
namespace vulkan {

  class RenderTarget : public purrr::RenderTarget {
  public:
    RenderTarget(Context *context, const RenderTargetInfo &info);
    ~RenderTarget();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    virtual std::pair<int, int> getSize() const override { return std::make_pair(mWidth, mHeight); }
  public:
    bool sameContext(Context *context) const { return mContext == context; }
  public:
    VkRenderPass  getRenderPass() const { return mRenderPass; }
    VkFramebuffer getFramebuffer() const { return mFramebuffer; }
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