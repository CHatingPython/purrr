#ifdef _PURRR_BACKEND_VULKAN

#ifndef _PURRR_VULKAN_IMAGE_HPP_
#define _PURRR_VULKAN_IMAGE_HPP_

#include "purrr/image.hpp"
#include "purrr/vulkan/context.hpp"

namespace purrr {
namespace vulkan {

  VkImageTiling vkImageTiling(ImageTiling tiling);

  class Image : public purrr::Image {
  public:
    Image(Context *context, const ImageInfo &info);
    ~Image();
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    virtual void copyData(size_t width, size_t height, size_t size, const void *data) override;
  public:
    Format          getFormat() const { return mFormat; }
    VkImage         getImage() const { return mImage; }
    VkImageView     getImageView() const { return mImageView; }
    VkDescriptorSet getDescriptorSet() const { return mDescriptorSet; }
  public:
    ImageInfo::Usage getUsage() const { return mUsage; }
  private:
    Context        *mContext       = nullptr;
    Format          mFormat        = Format::Undefined;
    VkImage         mImage         = VK_NULL_HANDLE;
    VkDeviceMemory  mMemory        = VK_NULL_HANDLE;
    VkImageView     mImageView     = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
  private:
    ImageInfo::Usage mUsage;
  private:
    VkImageLayout        mLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags mStage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags        mAccess = 0;
  private:
    void createImage(const ImageInfo &info);
    void allocateMemory();
    void createImageView(const ImageInfo &info);
    void allocateDescriptorSet(purrr::Sampler *sampler);
  public:
    void transitionImageLayout(
        VkImageLayout        dstLayout,
        VkPipelineStageFlags dstStage,
        VkAccessFlags        dstAccess,
        VkCommandBuffer      commandBuffer = VK_NULL_HANDLE);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_IMAGE_HPP_

#endif // _PURRR_BACKEND_VULKAN