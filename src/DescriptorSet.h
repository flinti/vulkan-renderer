#ifndef DESCRIPTORSET_H_
#define DESCRIPTORSET_H_

#include <map>
#include <vector>
#include <vulkan/vulkan.h>


class DescriptorPool;

class DescriptorSet
{
public:
    DescriptorSet(
        VkDevice device,
        DescriptorPool &descriptorPool, 
        std::map<uint32_t, VkDescriptorBufferInfo> bufferBindingInfos, 
        std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos
    );
    DescriptorSet(const DescriptorSet &) = delete;
    DescriptorSet(DescriptorSet &&);
    ~DescriptorSet();

    VkDescriptorSet getHandle() const;
    void updateAll();
private:
    VkDescriptorSet descriptorSet;
    std::map<uint32_t, VkDescriptorBufferInfo> bufferBindingInfos;
    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos;
    std::vector<VkWriteDescriptorSet> writes;

    DescriptorPool &descriptorPool;
    VkDevice device;
};

#endif
