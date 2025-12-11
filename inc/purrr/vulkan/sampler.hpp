#ifdef _PURRR_BACKEND_VULKAN

#ifndef _PURRR_VULKAN_SAMPLER_HPP_
#define _PURRR_VULKAN_SAMPLER_HPP_

#include "purrr/sampler.hpp"
#include "purrr/vulkan/context.hpp"

namespace purrr {
namespace vulkan {

  VkFilter             vkFilter(Filter filter);
  VkSamplerAddressMode vkSamplerAddressMode(SamplerAddressMode addressMode);

  class Sampler : public purrr::Sampler {
  public:
    Sampler(Context *context, const SamplerInfo &info);
    ~Sampler();
  public:
    virtual Api api() const override { return Api::Vulkan; }
  public:
    VkSampler getSampler() const { return mSampler; }
  private:
    Context  *mContext = nullptr;
    VkSampler mSampler = VK_NULL_HANDLE;
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_SAMPLER_HPP_

#endif // _PURRR_BACKEND_VULKAN