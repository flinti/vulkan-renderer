#include "DeviceAllocator.h"
#include "VkHelpers.h"
#include <cstddef>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

DeviceAllocator::DeviceAllocator(
    VkInstance instance, 
    VkPhysicalDevice physicalDevice, 
    VkDevice device,
    VkCommandPool immediateTransferPool,
    VkQueue immediateTransferQueue
)
    : instance(instance),
    physicalDevice(physicalDevice),
    device(device),
    immediateTransferPool(immediateTransferPool),
    immediateTransferQueue(immediateTransferQueue)
{
    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = instance;
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = device;

    VK_ASSERT(vmaCreateAllocator(&createInfo, &allocator));
}

DeviceAllocator::~DeviceAllocator()
{
    vmaDestroyAllocator(allocator);
}

std::pair<VkBuffer, VmaAllocation> DeviceAllocator::allocateDeviceLocalBufferAndTransfer(
    void *data,
    size_t size, 
    VkBufferUsageFlags usage
) {
    auto destinationBuf = allocateBuffer(
        size, 
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    auto stagingBuf = allocateBuffer(
        size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    vmaCopyMemoryToAllocation(
        allocator, 
        data, 
        stagingBuf.second, 
        0, 
        size
    );

    copyBuffer(stagingBuf.first, destinationBuf.first, size);
    vmaDestroyBuffer(allocator, stagingBuf.first, stagingBuf.second);

    return destinationBuf;
}

std::pair<VkImage, VmaAllocation> DeviceAllocator::allocateImageAttachment(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage
) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    allocationInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VK_ASSERT(vmaCreateImage(
        allocator, 
        &imageInfo, 
        &allocationInfo, 
        &image, 
        &allocation, 
        nullptr
    ));

    return std::make_pair(image, allocation);
}

void DeviceAllocator::free(std::pair<VkBuffer, VmaAllocation> allocation)
{
    vmaDestroyBuffer(allocator, allocation.first, allocation.second);
}

void DeviceAllocator::free(std::pair<VkImage, VmaAllocation> allocation)
{
    vmaDestroyImage(allocator, allocation.first, allocation.second);
}

std::pair<VkBuffer, VmaAllocation> DeviceAllocator::allocateBuffer(
    size_t size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties,
    VmaAllocationCreateFlags allocFlags
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = properties;
    allocInfo.flags = allocFlags;
    
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VK_ASSERT(vmaCreateBuffer(
        allocator, 
        &bufferInfo, 
        &allocInfo, 
        &buffer, 
        &allocation, 
        nullptr)
    );

    return std::make_pair(buffer, allocation);
}

void DeviceAllocator::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = immediateTransferPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VK_ASSERT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VK_ASSERT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_ASSERT(vkQueueSubmit(immediateTransferQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_ASSERT(vkQueueWaitIdle(immediateTransferQueue));

    vkFreeCommandBuffers(device, immediateTransferPool, 1, &commandBuffer);
}
