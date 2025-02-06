#include "Buffer.h"
#include "DeviceAllocator.h"

#include <fmt/format.h>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

Buffer::Buffer(DeviceAllocator &allocator, void *data, size_t size, VkBufferUsageFlags usage)
    : size(size),
    allocator(&allocator),
    bufferAllocation(VK_NULL_HANDLE, VK_NULL_HANDLE)
{
    bufferAllocation = allocator.allocateDeviceLocalBufferAndTransfer(data, size, usage);
}

Buffer::Buffer(Buffer &&b) noexcept
    : size(b.size),
    allocator(b.allocator),
    bufferAllocation(b.bufferAllocation)
{
    b.bufferAllocation.first = VK_NULL_HANDLE;
}

Buffer::~Buffer()
{
    if (bufferAllocation.first != VK_NULL_HANDLE) {
        allocator->free(bufferAllocation);
    }
}


Buffer &Buffer::operator =(Buffer &&other)
{
    size = other.size;
    allocator = other.allocator;
    bufferAllocation = other.bufferAllocation;

    other.allocator = nullptr;
    other.bufferAllocation.first = VK_NULL_HANDLE;

    return *this;
}


VkBuffer Buffer::getHandle() const
{
    return bufferAllocation.first;
}

size_t Buffer::getSize() const
{
    return size;
}



