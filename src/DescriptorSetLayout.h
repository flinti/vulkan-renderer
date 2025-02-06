#ifndef DESCRIPTORSETLAYOUT_H_
#define DESCRIPTORSETLAYOUT_H_

#include <map>
#include <vulkan/vulkan.h>
#include <vector>

class DescriptorSetLayout
{
public:
    DescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);
    DescriptorSetLayout(const DescriptorSetLayout &) = delete;
    DescriptorSetLayout(DescriptorSetLayout &&);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout getHandle() const;
    const std::vector<VkDescriptorSetLayoutBinding> &getBindings() const;
    const VkDescriptorSetLayoutBinding &getBinding(uint32_t bindingIndex) const;
private:
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::map<uint32_t, VkDescriptorSetLayoutBinding> bindingsKeyedByIndex;

    VkDevice device;
};

#endif
