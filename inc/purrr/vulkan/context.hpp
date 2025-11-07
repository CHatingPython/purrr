#ifndef _PURRR_VULKAN_CONTEXT_HPP_
#define _PURRR_VULKAN_CONTEXT_HPP_

#include "purrr/context.hpp"
#include "purrr/object.hpp"

#include "purrr/vulkan/program.hpp"

#include <queue>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h> // IWYU pragma: export

#include "purrr/platform.hpp"

namespace purrr {
namespace vulkan {

  VkIndexType vkIndexType(IndexType type);

  class Window;
  class Context : public purrr::platform::Context {
  public:
    Context(const ContextInfo &info);
    ~Context();
  public:
    Context(Context &&other)
      : purrr::platform::Context((purrr::platform::Context &&)other) {
      *this = std::move(other);
    }

    Context &operator=(Context &&other) {
      if (this == &other) return *this;

      mInstance         = other.mInstance;
      mPhysicalDevice   = other.mPhysicalDevice;
      mQueueFamilyIndex = other.mQueueFamilyIndex;
      mDevice           = other.mDevice;
      mQueue            = other.mQueue;

      other.mInstance         = VK_NULL_HANDLE;
      other.mPhysicalDevice   = VK_NULL_HANDLE;
      other.mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      other.mDevice           = VK_NULL_HANDLE;
      other.mQueue            = VK_NULL_HANDLE;

      return *this;
    }
  public:
    virtual constexpr Api api() const override { return Api::Vulkan; }
  public:
    virtual purrr::Window       *createWindow(const WindowInfo &info) override;
    virtual purrr::Buffer       *createBuffer(const BufferInfo &info) override;
    virtual purrr::Shader       *createShader(const ShaderInfo &info) override;
    virtual purrr::Sampler      *createSampler(const SamplerInfo &info) override;
    virtual purrr::Image        *createImage(const ImageInfo &info) override;
    virtual purrr::RenderTarget *createRenderTarget(const RenderTargetInfo &info) override;
  public:
    virtual purrr::Shader *createShader(ShaderType type, const std::vector<char> &code) override;
    virtual purrr::Shader *createShader(ShaderType type, const std::string_view &code) override;
  public:
    virtual void begin() override;
    virtual bool record(purrr::Window *window, const RecordClear &clear) override;
    virtual bool record(purrr::RenderTarget *renderTarget, const RecordClear &clear) override;
    virtual void useProgram(purrr::Program *program) override;
    virtual void useVertexBuffer(purrr::Buffer *buffer, uint32_t index) override;
    virtual void useIndexBuffer(purrr::Buffer *buffer, IndexType type) override;
    virtual void useUniformBuffer(purrr::Buffer *buffer, uint32_t index) override;
    virtual void useStorageBuffer(purrr::Buffer *buffer, uint32_t index) override;
    virtual void useTextureImage(purrr::Image *image, uint32_t index) override;
    virtual void draw(size_t vertexCount, size_t instanceCount) override;
    virtual void drawIndexed(size_t indexCount, size_t instanceCount) override;
    virtual void end() override;
    virtual void submit() override;
    virtual void present(bool preventSpinning) override;
    virtual void waitIdle() override;
  public:
    VkInstance       getInstance() const { return mInstance; }
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    uint32_t         getQueueFamilyIndex() const { return mQueueFamilyIndex; }
    VkDevice         getDevice() const { return mDevice; }
    VkQueue          getQueue() const { return mQueue; }
    VkCommandPool    getCommandPool() const { return mCommandPool; }
    VkCommandBuffer  getCommandBuffer() const { return mCommandBuffer; }
  public:
    VkDescriptorSetLayout getTextureDescriptorSetLayout() const { return mTextureDescriptorSetLayout; }
    VkDescriptorSetLayout getUniformDescriptorSetLayout() const { return mUniformDescriptorSetLayout; }
    VkDescriptorSetLayout getStorageDescriptorSetLayout() const { return mStorageDescriptorSetLayout; }
    VkDescriptorPool      getDescriptorPool() const { return mDescriptorPool; }
  private:
    VkInstance       mInstance         = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice   = VK_NULL_HANDLE;
    uint32_t         mQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkDevice         mDevice           = VK_NULL_HANDLE;
    VkQueue          mQueue            = VK_NULL_HANDLE;
    VkCommandPool    mCommandPool      = VK_NULL_HANDLE;
    VkCommandBuffer  mCommandBuffer    = VK_NULL_HANDLE;
    VkFence          mFence            = VK_NULL_HANDLE;
  private:
    VkDescriptorSetLayout mTextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mUniformDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout mStorageDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      mDescriptorPool             = VK_NULL_HANDLE;
  private: // Recorded windows
    std::vector<Window *>       mWindows          = {};
    std::vector<VkSwapchainKHR> mSwapchains       = {};
    std::vector<uint32_t>       mImageIndices     = {};
    std::vector<VkSemaphore>    mImageSemaphores  = {};
    std::vector<VkSemaphore>    mSubmitSemaphores = {};
    bool                        mRecording        = false;
    Program                    *mProgram          = nullptr;
    std::queue<Window *>        mRecreateQueue    = {};
  private:
    void createInstance(const ContextInfo &info);
    void chooseDevice(const std::vector<const char *> &extensions);
    void createDevice(const std::vector<const char *> &extensions);
    void getQueue();
    void createCommandPool();
    void allocateCommandBuffer();
    void createFence();
    void createDescriptorSetLayouts();
    void createDescriptorPool();
  private:
    virtual uint32_t scorePhysicalDevice(VkPhysicalDevice device);
    bool             deviceExtensionsPresent(VkPhysicalDevice device, const std::vector<const char *> extensions);
  protected:
    uint32_t findQueueFamily(VkPhysicalDevice device);
  public:
    uint32_t        findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer beginSingleTimeCommands();
    void            submitSingleTimeCommands(VkCommandBuffer commandBuffer);
  };

} // namespace vulkan
} // namespace purrr

#endif // _PURRR_VULKAN_CONTEXT_HPP_