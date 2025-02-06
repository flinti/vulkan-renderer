#include "DescriptorSetLayout.h"
#include "VkHelpers.h"

#include <stdexcept>
#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"
#include <utility>
#include <vulkan/vulkan.h>

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
    : device(device), bindings(bindings)
{
    VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(this->bindings.size());
	createInfo.pBindings = this->bindings.data();

    for (auto &binding : this->bindings) {
        bindingsKeyedByIndex[binding.binding] = binding;
    }

    VK_ASSERT(vkCreateDescriptorSetLayout(
        device, 
        &createInfo, 
        nullptr, 
        &descriptorSetLayout
    ));
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other)
    : device(other.device),
    descriptorSetLayout(other.descriptorSetLayout),
    bindings(std::move(other.bindings)),
    bindingsKeyedByIndex(std::move(other.bindingsKeyedByIndex))
{
    other.descriptorSetLayout = VK_NULL_HANDLE;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
}


VkDescriptorSetLayout DescriptorSetLayout::getHandle() const
{
    return descriptorSetLayout;
}

const std::vector<VkDescriptorSetLayoutBinding> &DescriptorSetLayout::getBindings() const
{
    return bindings;
}


const VkDescriptorSetLayoutBinding &DescriptorSetLayout::getBinding(uint32_t bindingIndex) const
{
    auto it = bindingsKeyedByIndex.find(bindingIndex);
    if (it == bindingsKeyedByIndex.end()) {
        throw std::invalid_argument(fmt::format("bindingIndex {} is out of range", bindingIndex));
    }

    return it->second;
}
