#include "Frame.h"
#include "DescriptorPool.h"
#include "VkHash.h"
#include "VkHelpers.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <vulkan/vulkan_core.h>

Frame::Frame(VkDevice device, uint32_t renderQueueFamilyIndex)
    : device(device)
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = renderQueueFamilyIndex;

	VK_ASSERT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));

    VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VK_ASSERT(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

	VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
    VK_ASSERT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
    VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
}

Frame::Frame(Frame &&other)
    : commandPool(other.commandPool),
    commandBuffer(other.commandBuffer),
    fence(other.fence),
    imageAvailableSemaphore(other.imageAvailableSemaphore),
    renderFinishedSemaphore(other.renderFinishedSemaphore),
    device(other.device)
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
    vkDestroyFence(device, fence, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
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
    size_t hash = hash_value(layout);

    auto &descriptorPoolMap = descriptorPools[concurrencyIndex];

    auto i = descriptorPoolMap.find(hash);
    if (i != descriptorPoolMap.end()) {
        return *i->second;
    }

    return *descriptorPoolMap.emplace(
        hash,
        std::make_unique<DescriptorPool>(device, layout)
    ).first->second;
}

DescriptorSet &Frame::getDescriptorSet(
    uint32_t concurrencyIndex,
    const DescriptorSetLayout &layout, 
    const std::map<uint32_t, VkDescriptorBufferInfo> &bufferBindingInfos, 
    const std::map<uint32_t, VkDescriptorImageInfo> &imageBindingInfos
)
{
    size_t hash = hash_value(layout);
    for (auto &i : bufferBindingInfos) {
        hash_combine(hash, i.second);
    }
    for (auto &i : imageBindingInfos) {
        hash_combine(hash, i.second);
    }

    auto &descriptorSetMap = descriptorSets[concurrencyIndex];

    auto i = descriptorSetMap.find(hash);
    if (i != descriptorSetMap.end()) {
        return *i->second;
    }
    DescriptorPool &pool = getDescriptorPool(concurrencyIndex, layout);

    return *descriptorSetMap.emplace(
        hash,
        std::make_unique<DescriptorSet>(device, pool, bufferBindingInfos, imageBindingInfos)
    ).first->second;
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

