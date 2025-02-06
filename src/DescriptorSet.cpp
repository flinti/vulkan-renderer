#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "VkHelpers.h"

#include <stdexcept>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>

DescriptorSet::DescriptorSet(
    VkDevice device,
    DescriptorPool &descriptorPool, 
    std::map<uint32_t, VkDescriptorBufferInfo> bufferBindingInfos, 
    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos
)
    : descriptorPool(descriptorPool),
    device(device),
    descriptorSet(descriptorPool.allocate()),
    bufferBindingInfos(std::move(bufferBindingInfos)),
    imageBindingInfos(std::move(imageBindingInfos))
{
}

DescriptorSet::DescriptorSet(DescriptorSet &&other)
    : descriptorPool(other.descriptorPool),
    device(other.device),
    descriptorSet(other.descriptorSet),
    bufferBindingInfos(std::move(other.bufferBindingInfos)),
    imageBindingInfos(std::move(other.imageBindingInfos))
{
    other.descriptorSet = VK_NULL_HANDLE;
}

DescriptorSet::~DescriptorSet()
{
    // do nothing because the set is managed by the pool
}


VkDescriptorSet DescriptorSet::getHandle() const
{
    return descriptorSet;
}

void DescriptorSet::updateAll()
{
    for (const auto &bufferInfo : bufferBindingInfos) {
        const auto &binding = descriptorPool.getDescriptorSetLayout().getBinding(bufferInfo.first);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = bufferInfo.first;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = binding.descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo.second;
        descriptorWrite.pImageInfo = nullptr;

        writes.push_back(descriptorWrite);
    }

    for (const auto &imageInfo : imageBindingInfos) {
        if (bufferBindingInfos.find(imageInfo.first) != bufferBindingInfos.end()) {
            throw std::invalid_argument(fmt::format(
                "attempting to update binding {} with an image info, but a buffer info for this binding index is also provided",
                imageInfo.first
            ));
        }

        const auto &binding = descriptorPool.getDescriptorSetLayout().getBinding(imageInfo.first);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = imageInfo.first;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = binding.descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &imageInfo.second;

        writes.push_back(descriptorWrite);
    }

    vkUpdateDescriptorSets(
        device, 
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}
