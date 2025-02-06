#ifndef MAPPEDBUFFER_H_
#define MAPPEDBUFFER_H_

#include "DeviceAllocator.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include "../third-party/VulkanMemoryAllocator/include/vk_mem_alloc.h"

class MappedBuffer
{
public:
    MappedBuffer(DeviceAllocator &allocator, size_t size, VkBufferUsageFlags usage);
    MappedBuffer(const MappedBuffer &) = delete;
    MappedBuffer(MappedBuffer &&) noexcept;
    ~MappedBuffer();

    VkBuffer getHandle() const;
    size_t getSize() const;
    std::byte *getData();
private:
    DeviceAllocator &allocator;

    std::byte *data;
    size_t size;
    std::pair<VkBuffer, VmaAllocation> bufferAllocation;
};

#endif
