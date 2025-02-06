#include "DescriptorPool.h"
#include "VkHelpers.h"

#include <map>
#include <utility>
#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"
#include <vulkan/vulkan.h>

DescriptorPool::DescriptorPool(VkDevice device, const DescriptorSetLayout &layout, uint32_t poolSize)
    : device(device), layout(layout), poolMaxSize(poolSize)
{
    std::map<VkDescriptorType, uint32_t> descriptorCountPerType;

    for (const auto &binding : layout.getBindings()) {
        descriptorCountPerType[binding.descriptorType] += binding.descriptorCount;
    }

    for (const auto &descriptorCount : descriptorCountPerType) {
        auto &poolData = poolSizes.emplace_back(VkDescriptorPoolSize{
            .type = descriptorCount.first,
            .descriptorCount = descriptorCount.second * poolSize,
        });
    }
}

DescriptorPool::DescriptorPool(DescriptorPool &&other)
    : device(other.device),
    layout(other.layout),
    poolMaxSize(other.poolMaxSize),
    currentPoolIndex(other.currentPoolIndex),
    poolSizes(std::move(other.poolSizes)),
    pools(std::move(other.pools))
{
}

DescriptorPool::~DescriptorPool()
{
    destroyPools();
}

void DescriptorPool::destroyPools()
{
    for (auto &pool : pools) {
        vkDestroyDescriptorPool(device, pool.pool, nullptr);
    }
}

void DescriptorPool::reset()
{
    destroyPools();
    pools.clear();
    currentPoolIndex = 0;
}

VkDescriptorSet DescriptorPool::allocate()
{
    PoolData &poolData = getPoolData();

    auto setLayout = layout.getHandle();

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = poolData.pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &setLayout;

    VkDescriptorSet descriptorSet;
    VK_ASSERT(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet));

    ++poolData.allocatedSetCount;

    return descriptorSet;
}

const DescriptorSetLayout &DescriptorPool::getDescriptorSetLayout() const
{
    return layout;
}

DescriptorPool::PoolData &DescriptorPool::getPoolData()
{
    if (pools.size() <= currentPoolIndex) {
        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.maxSets = poolMaxSize;
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.poolSizeCount = poolSizes.size();

        VkDescriptorPool pool = VK_NULL_HANDLE;
        VK_ASSERT(vkCreateDescriptorPool(
            device, 
            &createInfo, 
            nullptr, 
            &pool
        ));

        PoolData data = {
            .pool = pool,
            .allocatedSetCount = 0,
        };

        pools.push_back(data);

        return pools[currentPoolIndex];
    }
    else if (pools[currentPoolIndex].allocatedSetCount < poolMaxSize) {
        return pools[currentPoolIndex];
    }

    // maximum pool size exceeded -> allocate new one
    ++currentPoolIndex;
    return getPoolData();
}
