#ifndef DEVICEALLOCATOR_H_
#define DEVICEALLOCATOR_H_

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>
#include "../third-party/VulkanMemoryAllocator/include/vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class Device;

class DeviceAllocator
{
public:
    DeviceAllocator(
        VkInstance instance, 
        VkPhysicalDevice physicalDevice, 
        VkDevice device,
        VkCommandPool immediateTransferPool,
        VkQueue immediateTransferQueue
    );
    ~DeviceAllocator();

    std::pair<VkBuffer, VmaAllocation> allocateHostVisibleCoherentAndMap(
        size_t size, 
        VkBufferUsageFlags usage,
        void **mappedData
    );
    std::pair<VkBuffer, VmaAllocation> allocateDeviceLocalBufferAndTransfer(
        void *data,
        size_t size, 
        VkBufferUsageFlags usage
    );
    std::pair<VkImage, VmaAllocation> allocateDeviceLocalImageAndTransfer(
        void *data,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage
    );
    std::pair<VkImage, VmaAllocation> allocateImageAttachment(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage
    );
    void free(std::pair<VkBuffer, VmaAllocation> allocation);
    void free(std::pair<VkImage, VmaAllocation> allocation);
private:
    void withThrowawayCommandBuffer(std::function<void (VkCommandBuffer)> recorder);
    std::pair<VkBuffer, VmaAllocation> allocateBuffer(
        size_t size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties,
        VmaAllocationCreateFlags allocFlags = 0,
        VmaAllocationInfo *allocInfo = nullptr
    );
    std::pair<VkImage, VmaAllocation> allocateImageAsTransferDst(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties,
        VmaAllocationCreateFlags allocFlags = 0
    );
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void enqueueImageLayoutTransition(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkAccessFlags srcMask, 
        VkAccessFlags dstMask, 
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage
    );
    void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);

    VmaAllocator allocator = VK_NULL_HANDLE;

    VkCommandPool immediateTransferPool;
    VkQueue immediateTransferQueue;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};

#endif
