#include "MappedBuffer.h"
#include "DeviceAllocator.h"

#include <cstring>
#include <fmt/format.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

MappedBuffer::MappedBuffer(DeviceAllocator &allocator, size_t size, VkBufferUsageFlags usage)
    : allocator(allocator),
    data(nullptr),
    size(size),
    bufferAllocation(allocator.allocateHostVisibleCoherentAndMap(size, usage, (void **) &data))
{
    std::memset(data, 0, size);
}

MappedBuffer::MappedBuffer(MappedBuffer &&b) noexcept
    : allocator(b.allocator),
    bufferAllocation(b.bufferAllocation)
{
    b.bufferAllocation.first = VK_NULL_HANDLE;
}

MappedBuffer::~MappedBuffer()
{
    if (bufferAllocation.first != VK_NULL_HANDLE) {
        allocator.free(bufferAllocation);
    }
}

VkBuffer MappedBuffer::getHandle() const
{
    return bufferAllocation.first;
}

size_t MappedBuffer::getSize() const
{
    return size;
}

std::byte *MappedBuffer::getData()
{
    return data;
}



