#include "VulkanObjectCache.h"
#include "DescriptorSetLayout.h"
#include "Device.h"
#include "Resource.h"
#include "VkHelpers.h"
#include "VkHash.h"
#include <memory>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>

VulkanObjectCache::VulkanObjectCache(Device &device)
    : device(device)
{
}

VulkanObjectCache::~VulkanObjectCache()
{
    for (auto &sampler : samplers) {
        vkDestroySampler(device.getDeviceHandle(), sampler.second, nullptr);
    }
}


VkSampler VulkanObjectCache::getSampler(const VkSamplerCreateInfo &info)
{
    size_t hash = Utility::hash_value(info);
    auto i = samplers.find(hash);
    if (i != samplers.end()) {
        return i->second;
    }
    VkSampler newSampler = VK_NULL_HANDLE;
    VK_ASSERT(vkCreateSampler(device.getDeviceHandle(), &info, nullptr, &newSampler));
    samplers.emplace(hash, newSampler);

    spdlog::info("VulkanObjectCache: created sampler {}", (void*) newSampler);

    return newSampler;
}

DescriptorSetLayout &VulkanObjectCache::getDescriptorSetLayout(
    const std::vector<VkDescriptorSetLayoutBinding> &bindings,
    VkDescriptorSetLayoutCreateFlags flags
)
{
    size_t hash = Utility::hash_value(flags);
    for (auto &binding : bindings) {
        Utility::hash_combine(hash, binding);
    }
    auto i = descriptorSetLayouts.find(hash);
    if (i != descriptorSetLayouts.end()) {
        return *i->second;
    }

    DescriptorSetLayout &layout = *descriptorSetLayouts.emplace(
        hash,
        std::make_unique<DescriptorSetLayout>(device.getDeviceHandle(), bindings)
    ).first->second;

    spdlog::info("VulkanObjectCache: created descriptor set layout at {}", (void*) &layout);

    return layout;
}


Image &VulkanObjectCache::getImage(const ImageResource &resource)
{
    ResourceId id = resource.getId();
    auto i = images.find(id);
    if (i != images.end()) {
        return *i->second;
    }

    Image &image = *images.emplace(
        id,
        std::make_unique<Image>(device, resource)
    ).first->second;

    spdlog::info("VulkanObjectCache: created Image at {}", (void*) &image);

    return image;
}

Shader &VulkanObjectCache::getShader(const ShaderResource &resource)
{
    ResourceId id = resource.getId();
    auto i = shaders.find(id);
    if (i != shaders.end()) {
        return *i->second;
    }

    Shader &shader = *shaders.emplace(
        id,
        std::make_unique<Shader>(device, resource)
    ).first->second;

    spdlog::info("VulkanObjectCache: created Shader at {}", (void*) &shader);

    return shader;
}
