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
  auto createInfo = VkSamplerCreateInfo{ .sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                         .pNext            = VK_NULL_HANDLE,
                                         .flags            = 0,
                                         .magFilter        = vkFilter(info.magFilter),
                                         .minFilter        = vkFilter(info.minFilter),
                                         .mipmapMode       = static_cast<VkSamplerMipmapMode>(vkFilter(info.mipFilter)),
                                         .addressModeU     = vkSamplerAddressMode(info.addressModeU),
                                         .addressModeV     = vkSamplerAddressMode(info.addressModeV),
                                         .addressModeW     = vkSamplerAddressMode(info.addressModeW),
                                         .mipLodBias       = 0.0f,
                                         .anisotropyEnable = VK_FALSE,
                                         .maxAnisotropy    = 0.0f,
                                         .compareEnable    = VK_FALSE,
                                         .compareOp        = VK_COMPARE_OP_NEVER,
                                         .minLod           = 0.0f,
                                         .maxLod           = 1.0f,
                                         .borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                                         .unnormalizedCoordinates = VK_FALSE };

  expectResult("Sampler creation", vkCreateSampler(mContext->getDevice(), &createInfo, VK_NULL_HANDLE, &mSampler));
}

Sampler::~Sampler() {
  if (mSampler != VK_NULL_HANDLE) vkDestroySampler(mContext->getDevice(), mSampler, VK_NULL_HANDLE);
}

} // namespace purrr::vulkan