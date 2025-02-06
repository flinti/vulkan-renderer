#include "Frame.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "RenderObject.h"
#include "VkHash.h"
#include "VkHelpers.h"

#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <utility>
#include <vulkan/vulkan.h>

Frame::Frame(Device &device, uint32_t renderQueueFamilyIndex)
    : device(device),
    globalUniformBuffer(createGlobalUniformBuffer()),
    globalUniformDataDescriptorSet(getDescriptorSet(
        0, 
        device.getObjectCache().getDescriptorSetLayout(RenderObject::getGlobalUniformDataLayoutBindings()), 
        std::map<uint32_t, VkDescriptorBufferInfo>{std::make_pair(
            0,
            VkDescriptorBufferInfo{
                .buffer = globalUniformBuffer.getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            }
        )}, 
        {})
    )
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = renderQueueFamilyIndex;

	VK_ASSERT(vkCreateCommandPool(
        device.getDeviceHandle(), 
        &poolInfo, 
        nullptr, 
        &commandPool)
    );

    VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_ASSERT(vkAllocateCommandBuffers(
        device.getDeviceHandle(), 
        &allocInfo, 
        &commandBuffer)
    );

	VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateSemaphore(
        device.getDeviceHandle(), 
        &semaphoreInfo, nullptr, 
        &imageAvailableSemaphore)
    );
    VK_ASSERT(vkCreateSemaphore(
        device.getDeviceHandle(), 
        &semaphoreInfo, nullptr, 
        &renderFinishedSemaphore)
    );
    VK_ASSERT(vkCreateFence(
        device.getDeviceHandle(), 
        &fenceInfo, 
        nullptr, &fence)
    );
}

Frame::Frame(Frame &&other)
    : device(other.device),
    commandPool(other.commandPool),
    commandBuffer(other.commandBuffer),
    fence(other.fence),
    imageAvailableSemaphore(other.imageAvailableSemaphore),
    renderFinishedSemaphore(other.renderFinishedSemaphore),
    globalUniformBuffer(std::move(other.globalUniformBuffer)),
    descriptorPools(std::move(other.descriptorPools)),
    descriptorSets(std::move(other.descriptorSets)),
    globalUniformDataDescriptorSet(other.globalUniformDataDescriptorSet)
{
    other.commandPool = VK_NULL_HANDLE;
    other.commandBuffer = VK_NULL_HANDLE;
    other.fence = VK_NULL_HANDLE;
    other.imageAvailableSemaphore = VK_NULL_HANDLE;
    other.renderFinishedSemaphore = VK_NULL_HANDLE;
}

Frame::~Frame()
{
    for (auto &map : descriptorSets) {
        for (auto &i : map.second) {
            i.second.reset();
        }
    }
    for (auto &map : descriptorPools) {
        for (auto &i : map.second) {
            i.second.reset();
        }
    }
    vkDestroyFence(device.getDeviceHandle(), fence, nullptr);
    vkDestroySemaphore(device.getDeviceHandle(), renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device.getDeviceHandle(), imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(device.getDeviceHandle(), commandPool, nullptr);
}

const VkCommandBuffer &Frame::getCommandBuffer() const
{
    return commandBuffer;
}

const VkFence &Frame::getFence() const
{
    return fence;
}

const VkSemaphore &Frame::getImageAvailableSemaphore() const
{
    return imageAvailableSemaphore;
}

const VkSemaphore &Frame::getRenderFinishedSemaphore() const
{
    return renderFinishedSemaphore;
}

DescriptorPool &Frame::getDescriptorPool(uint32_t concurrencyIndex, const DescriptorSetLayout &layout)
{
    size_t hash = Utility::hash_value(layout);

    auto &descriptorPoolMap = descriptorPools[concurrencyIndex];

    auto i = descriptorPoolMap.find(hash);
    if (i != descriptorPoolMap.end()) {
        return *i->second;
    }

    auto &ret = *descriptorPoolMap.emplace(
        hash,
        std::make_unique<DescriptorPool>(device.getDeviceHandle(), layout)
    ).first->second;
    spdlog::info("Frame: created descriptor pool at {}", (void*) &ret);
    return ret;
}

DescriptorSet &Frame::getDescriptorSet(
    uint32_t concurrencyIndex,
    const DescriptorSetLayout &layout, 
    const std::map<uint32_t, VkDescriptorBufferInfo> &bufferBindingInfos, 
    const std::map<uint32_t, VkDescriptorImageInfo> &imageBindingInfos
)
{
    size_t hash = Utility::hash_value(layout);
    for (auto &i : bufferBindingInfos) {
        Utility::hash_combine(hash, i.second);
    }
    for (auto &i : imageBindingInfos) {
        Utility::hash_combine(hash, i.second);
    }

    auto &descriptorSetMap = descriptorSets[concurrencyIndex];

    auto i = descriptorSetMap.find(hash);
    if (i != descriptorSetMap.end()) {
        return *i->second;
    }
    DescriptorPool &pool = getDescriptorPool(concurrencyIndex, layout);

    auto &ret = *descriptorSetMap.emplace(
        hash,
        std::make_unique<DescriptorSet>(
            device.getDeviceHandle(), 
            pool, bufferBindingInfos, 
            imageBindingInfos
        )
    ).first->second;
    spdlog::info("Frame: created descriptor set at {}", (void*) &ret);
    return ret;
}


DescriptorSet &Frame::getGlobalUniformDataDescriptorSet()
{
    return globalUniformDataDescriptorSet;
}

void Frame::updateDescriptorSets(uint32_t concurrencyIndex)
{
    auto map = descriptorSets.find(concurrencyIndex);
    if (map != descriptorSets.end()) {
        for (auto &set : map->second) {
            set.second->updateAll();
        }
    }
}

void Frame::updateGlobalUniformBuffer(const GlobalUniformData &data)
{
    *reinterpret_cast<GlobalUniformData *>(globalUniformBuffer.getData()) = data;
}

GlobalUniformData &Frame::getGlobalUniformData()
{
    return *reinterpret_cast<GlobalUniformData *>(globalUniformBuffer.getData());
}

VkBuffer Frame::getGlobalUniformBufferHandle()
{
    return globalUniformBuffer.getHandle();
}


MappedBuffer Frame::createGlobalUniformBuffer()
{
    return MappedBuffer(
        device.getAllocator(),
        sizeof(GlobalUniformData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
}
