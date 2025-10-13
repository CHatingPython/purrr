#include "purrr/vulkan/context.hpp"

namespace purrr::vulkan {

Context::Context(const ContextInfo &info) {
  createInstance(info);
  chooseDevice();
  findQueueFamily();
  createDevice();
  getQueue();
}

Context::~Context() {}

void Context::createInstance(const ContextInfo &info) {}

void Context::chooseDevice() {}

void Context::findQueueFamily() {}

void Context::createDevice() {}

void Context::getQueue() {}

uint32_t Context::scorePhysicalDevice(VkPhysicalDevice device) {
  return 0;
}

} // namespace purrr::vulkan