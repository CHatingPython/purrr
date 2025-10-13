#include "purrr/vulkan/exceptions.hpp"

#include "purrr/vulkan/context.hpp"

#include <vector>
#include <unordered_set>

namespace purrr::vulkan {

Context::Context(const ContextInfo &info) {
  auto deviceExtensions = std::vector<const char *>();

  createInstance(info);
  chooseDevice(deviceExtensions);
  createDevice(deviceExtensions);
  getQueue();
}

Context::~Context() {
  if (mDevice != VK_NULL_HANDLE) vkDestroyDevice(mDevice, VK_NULL_HANDLE);
  if (mInstance != VK_NULL_HANDLE) vkDestroyInstance(mInstance, VK_NULL_HANDLE);
}

void Context::createInstance(const ContextInfo &info) {
  auto layers     = std::vector<const char *>();
  auto extensions = std::vector<const char *>();

  if (true) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  { // Check if every required layer is present
    uint32_t count = 0;
    expectResult("Instance layer enumeration", vkEnumerateInstanceLayerProperties(&count, VK_NULL_HANDLE));

    auto availableLayers = std::vector<VkLayerProperties>(count);
    expectResult("Instance layer enumeration", vkEnumerateInstanceLayerProperties(&count, availableLayers.data()));

    auto requiredLayers = std::unordered_set<std::string_view>(layers.begin(), layers.end());
    for (const VkLayerProperties &layer : availableLayers) {
      requiredLayers.erase(layer.layerName);
    }

    if (!requiredLayers.empty()) throw NotPresent("layers", requiredLayers);
  }

  { // Check if every required extension is present
    uint32_t count = 0;
    expectResult(
        "Instance extension enumeration",
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &count, VK_NULL_HANDLE));

    auto availableExtensions = std::vector<VkExtensionProperties>(count);
    expectResult(
        "Instance extension enumeration",
        vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &count, availableExtensions.data()));

    auto requiredExtensions = std::unordered_set<std::string_view>(extensions.begin(), extensions.end());
    for (const VkExtensionProperties &extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) throw NotPresent("extensions", requiredExtensions);
  }

  auto applicationInfo = VkApplicationInfo{ .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                            .pNext              = VK_NULL_HANDLE,
                                            .pApplicationName   = info.appName,
                                            .applicationVersion = info.appVersion,
                                            .pEngineName        = info.engineName,
                                            .engineVersion      = info.engineVersion,
                                            .apiVersion         = info.apiVersion };

  auto createInfo = VkInstanceCreateInfo{ .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                          .pNext                   = VK_NULL_HANDLE,
                                          .flags                   = 0,
                                          .pApplicationInfo        = &applicationInfo,
                                          .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
                                          .ppEnabledLayerNames     = layers.data(),
                                          .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
                                          .ppEnabledExtensionNames = extensions.data() };

  expectResult("Instance creation", vkCreateInstance(&createInfo, VK_NULL_HANDLE, &mInstance));
}

void Context::chooseDevice(const std::vector<const char *> &extensions) {
  uint32_t count = 0;
  expectResult("Physical device enumeration", vkEnumeratePhysicalDevices(mInstance, &count, VK_NULL_HANDLE));

  auto devices = std::vector<VkPhysicalDevice>(count);
  expectResult("Physical device enumeration", vkEnumeratePhysicalDevices(mInstance, &count, devices.data()));

  /*
   0 - every device scored 0
   1 - no device that scored more than 0, had a graphics queue
   2 - no suitable device had all required extensions present
   */
  uint32_t found = 0;

  uint32_t bestScore = 0;
  for (VkPhysicalDevice device : devices) {
    uint32_t score = scorePhysicalDevice(device);
    if (score <= bestScore) continue;

    uint32_t queueFamilyIndex = findQueueFamily(device);
    if (queueFamilyIndex == VK_QUEUE_FAMILY_IGNORED) {
      if (found < 1) found = 1;
      continue;
    }

    if (!deviceExtensionsPresent(device, extensions)) {
      found = 2;
      continue;
    }

    bestScore         = score;
    mPhysicalDevice   = device;
    mQueueFamilyIndex = queueFamilyIndex;
  }

  if (!mPhysicalDevice) {
    switch (found) {
    case 0:
    case 1: throw std::runtime_error("No suitable devices found");
    case 2: throw std::runtime_error("No suitable device had every required extension present");
    default: throw std::runtime_error("Something went wrong...");
    }
  }
}

void Context::createDevice(const std::vector<const char *> &extensions) {
  auto features = VkPhysicalDeviceFeatures{};

  float priorities      = 0.0f;
  auto  queueCreateInfo = VkDeviceQueueCreateInfo{ .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                   .pNext            = VK_NULL_HANDLE,
                                                   .flags            = 0,
                                                   .queueFamilyIndex = mQueueFamilyIndex,
                                                   .queueCount       = 1,
                                                   .pQueuePriorities = &priorities };

  auto createInfo = VkDeviceCreateInfo{ .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                        .pNext                   = VK_NULL_HANDLE,
                                        .flags                   = 0,
                                        .queueCreateInfoCount    = 1,
                                        .pQueueCreateInfos       = &queueCreateInfo,
                                        .enabledLayerCount       = 0,
                                        .ppEnabledLayerNames     = VK_NULL_HANDLE,
                                        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
                                        .ppEnabledExtensionNames = extensions.data(),
                                        .pEnabledFeatures        = &features };

  expectResult("Device creation", vkCreateDevice(mPhysicalDevice, &createInfo, VK_NULL_HANDLE, &mDevice));
}

void Context::getQueue() {
  vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);
}

uint32_t Context::scorePhysicalDevice(VkPhysicalDevice device) {
  uint32_t score = 0;

  auto properties = VkPhysicalDeviceProperties{};
  vkGetPhysicalDeviceProperties(device, &properties);

  switch (properties.deviceType) {
  case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
    score += 50;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
    score += 100;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
    score += 10;
    break;
  }
  case VK_PHYSICAL_DEVICE_TYPE_CPU: {
    score += 25;
    break;
  }
  default: break;
  }

  return score;
}

bool Context::deviceExtensionsPresent(VkPhysicalDevice device, const std::vector<const char *> extensions) {
  uint32_t count = 0;
  expectResult(
      "Device extension enumeration",
      vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &count, VK_NULL_HANDLE));

  auto availableExtensions = std::vector<VkExtensionProperties>(count);
  expectResult(
      "Device extension enumeration",
      vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &count, availableExtensions.data()));

  auto requiredExtensions = std::unordered_set<std::string_view>(extensions.begin(), extensions.end());
  for (const VkExtensionProperties &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

uint32_t Context::findQueueFamily(VkPhysicalDevice device) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);

  auto properties = std::vector<VkQueueFamilyProperties>(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

  for (uint32_t familyIndex = 0; familyIndex < properties.size(); ++familyIndex) {
    if (properties[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) return familyIndex;
  }

  return VK_QUEUE_FAMILY_IGNORED;
}

} // namespace purrr::vulkan