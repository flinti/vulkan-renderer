#ifndef BUFFER_H_
#define BUFFER_H_

#include "DeviceAllocator.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include "../third-party/VulkanMemoryAllocator/include/vk_mem_alloc.h"

class Buffer
{
public:
    Buffer(DeviceAllocator &allocator, void *data, size_t size, VkBufferUsageFlags usage);
    Buffer(const Buffer &) = delete;
    Buffer(Buffer &&) noexcept;
    ~Buffer();

    Buffer &operator =(Buffer &&);

    VkBuffer getHandle() const;
    size_t getSize() const;
private:
    size_t size;
    std::pair<VkBuffer, VmaAllocation> bufferAllocation;

    DeviceAllocator *allocator;
};

#endif
