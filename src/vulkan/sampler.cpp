#ifdef _PURRR_BACKEND_VULKAN

#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/sampler.hpp"

#include <stdexcept>

namespace purrr::vulkan {

VkFilter vkFilter(Filter filter) {
  switch (filter) {
  case Filter::Nearest: return VK_FILTER_NEAREST;
  case Filter::Linear: return VK_FILTER_LINEAR;
  }

  throw std::runtime_error("Unreachable");
}

VkSamplerAddressMode vkSamplerAddressMode(SamplerAddressMode addressMode) {
  switch (addressMode) {
  case SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case SamplerAddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case SamplerAddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case SamplerAddressMode::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  }

  throw std::runtime_error("Unreachable");
}

Sampler::Sampler(Context *context, const SamplerInfo &info)
  : mContext(context) {
  VkSamplerCreateInfo createInfo{};
  createInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.pNext                   = VK_NULL_HANDLE;
  createInfo.flags                   = 0;
  createInfo.magFilter               = vkFilter(info.magFilter);
  createInfo.minFilter               = vkFilter(info.minFilter);
  createInfo.mipmapMode              = static_cast<VkSamplerMipmapMode>(vkFilter(info.mipFilter));
  createInfo.addressModeU            = vkSamplerAddressMode(info.addressModeU);
  createInfo.addressModeV            = vkSamplerAddressMode(info.addressModeV);
  createInfo.addressModeW            = vkSamplerAddressMode(info.addressModeW);
  createInfo.mipLodBias              = 0.0f;
  createInfo.anisotropyEnable        = VK_FALSE;
  createInfo.maxAnisotropy           = 0.0f;
  createInfo.compareEnable           = VK_FALSE;
  createInfo.compareOp               = VK_COMPARE_OP_NEVER;
  createInfo.minLod                  = 0.0f;
  createInfo.maxLod                  = 1.0f;
  createInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  createInfo.unnormalizedCoordinates = VK_FALSE;

  expectResult("Sampler creation", vkCreateSampler(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mSampler));
}

Sampler::~Sampler() {
  if (mSampler != VK_NULL_HANDLE) vkDestroySampler(mContext->getDevice(), mSampler, VK_NULL_HANDLE);
}

} // namespace purrr::vulkan

#endif // _PURRR_BACKEND_VULKAN