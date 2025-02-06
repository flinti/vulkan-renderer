#include "DeviceAllocator.h"
#include "VkHelpers.h"
#include <cstddef>
#include <cstdint>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <utility>
#include "../third-party/VulkanMemoryAllocator/include/vk_mem_alloc.h"
#include <vulkan/vulkan.h>

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
    spdlog::info("DeviceAllocator: creating vma allocator...");

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

std::pair<VkBuffer, VmaAllocation> DeviceAllocator::allocateHostVisibleCoherentAndMap(
    size_t size, 
    VkBufferUsageFlags usage,
    void **mappedData
) {
    VmaAllocationInfo allocInfo{};
    auto buf = allocateBuffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        &allocInfo
    );
    *mappedData = allocInfo.pMappedData;

    return buf;
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

std::pair<VkImage, VmaAllocation> DeviceAllocator::allocateDeviceLocalImageAndTransfer(
    void *data,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage
) {
    auto destinationImage = allocateImageAsTransferDst(
        width,
        height, 
        format,
        usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    size_t size = width * height * 4; // RGBA
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

    copyBufferToImage(stagingBuf.first, destinationImage.first, width, height);
    vmaDestroyBuffer(allocator, stagingBuf.first, stagingBuf.second);

    return destinationImage;
}

void DeviceAllocator::free(std::pair<VkBuffer, VmaAllocation> allocation)
{
    vmaDestroyBuffer(allocator, allocation.first, allocation.second);
}

void DeviceAllocator::free(std::pair<VkImage, VmaAllocation> allocation)
{
    vmaDestroyImage(allocator, allocation.first, allocation.second);
}


void DeviceAllocator::withThrowawayCommandBuffer(std::function<void (VkCommandBuffer)> recorder)
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

    recorder(commandBuffer);

    VK_ASSERT(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_ASSERT(vkQueueSubmit(immediateTransferQueue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_ASSERT(vkQueueWaitIdle(immediateTransferQueue));

    vkFreeCommandBuffers(device, immediateTransferPool, 1, &commandBuffer);
}

std::pair<VkBuffer, VmaAllocation> DeviceAllocator::allocateBuffer(
    size_t size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties,
    VmaAllocationCreateFlags allocFlags,
    VmaAllocationInfo *allocInfo
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.requiredFlags = properties;
    allocCreateInfo.flags = allocFlags;
    
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VK_ASSERT(vmaCreateBuffer(
        allocator, 
        &bufferInfo, 
        &allocCreateInfo, 
        &buffer, 
        &allocation, 
        allocInfo)
    );

    return std::make_pair(buffer, allocation);
}

std::pair<VkImage, VmaAllocation> DeviceAllocator::allocateImageAsTransferDst(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties,
    VmaAllocationCreateFlags allocFlags
) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.requiredFlags = properties;
    allocInfo.flags = allocFlags;
    
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VK_ASSERT(vmaCreateImage(
        allocator, 
        &imageInfo, 
        &allocInfo, 
        &image, 
        &allocation, 
        nullptr)
    );

    return std::make_pair(image, allocation);
}

void DeviceAllocator::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t size)
{
    withThrowawayCommandBuffer([=](VkCommandBuffer commandBuffer) -> void {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

void DeviceAllocator::enqueueImageLayoutTransition(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccess, 
    VkAccessFlags dstAccess, 
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage
) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(commandBuffer,
        srcStage,
        dstStage,
        0,
        0,
        nullptr,
        0, 
        nullptr, 
        1, 
        &barrier
    );
}

void DeviceAllocator::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
    withThrowawayCommandBuffer([=](VkCommandBuffer commandBuffer) -> void {
        enqueueImageLayoutTransition(
            commandBuffer,
            dstImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, 
            VK_ACCESS_TRANSFER_WRITE_BIT, 
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
            VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = { width, height, 1 };
        vkCmdCopyBufferToImage(
            commandBuffer, 
            srcBuffer, 
            dstImage, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            1, 
            &region
        );

        enqueueImageLayoutTransition(
            commandBuffer,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, 
            VK_ACCESS_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    });
}
