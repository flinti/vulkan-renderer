#ifndef DESCRIPTORPOOL_H_
#define DESCRIPTORPOOL_H_

#include "DescriptorSetLayout.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vulkan/vulkan.h>
#include <vector>

class DescriptorPool
{
public:
    DescriptorPool(VkDevice device, const DescriptorSetLayout &layout, uint32_t poolSize = 16);
    DescriptorPool(const DescriptorPool &) = delete;
    DescriptorPool(DescriptorPool &&);
    ~DescriptorPool();

    void reset();
    VkDescriptorSet allocate();
    const DescriptorSetLayout &getDescriptorSetLayout() const;
private:
    void destroyPools();

    struct PoolData
    {
        VkDescriptorPool pool;
        uint32_t allocatedSetCount;
    };

    PoolData &getPoolData();

    uint32_t poolMaxSize;
    uint32_t currentPoolIndex = 0;
    std::vector<VkDescriptorPoolSize> poolSizes;
    std::vector<PoolData> pools;

    VkDevice device;
    const DescriptorSetLayout &layout;
};

#endif
