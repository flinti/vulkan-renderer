#ifndef DEVICE_H_
#define DEVICE_H_

#include "DeviceAllocator.h"
#include "SwapChain.h"
#include "VulkanObjectCache.h"

#include <memory>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>
#include <optional>

class Instance;
class DeviceAllocator;

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() const
    {
        return graphics.has_value() && present.has_value();
    }
};

class Device
{
public:
    Device(
        Instance &instance, 
        VkSurfaceKHR surface, 
        std::vector<const char *> extensionsToEnable = {}
    );
    Device(const Device &) = delete;
    Device(Device &&) = delete;
    ~Device();

    VulkanObjectCache &getObjectCache();
    DeviceAllocator &getAllocator();

    VkInstance getInstanceHandle();
    VkPhysicalDevice getPhysicalDeviceHandle();
    VkDevice getDeviceHandle();
    VkQueue getGraphicsQueue();
    VkQueue getPresentQueue();
    const QueueFamilyIndices &getQueueFamilyIndices() const;

    void waitDeviceIdle();
private:
    void createInstance();
    bool checkValidationLayersSupported();
    VkPhysicalDevice chooseSuitablePhysicalDevice();
    bool isDeviceSuitable(
        VkPhysicalDevice device, 
        const QueueFamilyIndices &queueFamilyIndices, 
        const SwapChainSupportDetails &swapChainSupportDetails, 
        const VkPhysicalDeviceProperties &deviceProperties, 
        const VkPhysicalDeviceFeatures &deviceFeatures
    );
    bool checkDeviceRequiredExtensionsSupport(VkPhysicalDevice device);
    QueueFamilyIndices findNeededQueueFamilyIndices(VkPhysicalDevice device);
    VkDevice createLogicalDevice();
    VkCommandPool createTransferCommandPool();

    Instance &instance;
    VkSurfaceKHR surface;
    std::vector<const char *> extensionsToEnable;
    QueueFamilyIndices selectedQueueFamilyIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkCommandPool transferCommandPool;
    std::unique_ptr<DeviceAllocator> allocator;
    std::unique_ptr<VulkanObjectCache> objectCache;
};

#endif
