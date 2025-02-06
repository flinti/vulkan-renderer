#ifndef VULKANOBJECTCACHE_H_
#define VULKANOBJECTCACHE_H_

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "Image.h"
#include "Resource.h"
#include "Shader.h"
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.h>

class Device;

class VulkanObjectCache
{
public:
    typedef size_t KeyType;

    VulkanObjectCache(Device &device);
    VulkanObjectCache(const VulkanObjectCache &) = delete;
    ~VulkanObjectCache();

    VkSampler getSampler(const VkSamplerCreateInfo &info);
    DescriptorSetLayout &getDescriptorSetLayout(
        const std::vector<VkDescriptorSetLayoutBinding> &bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0
    );
    Image &getImage(const ImageResource &resource);
    Shader &getShader(const ShaderResource &resource);
private:
    Device &device;
    
    std::unordered_map<KeyType, VkSampler> samplers;
    std::unordered_map<KeyType, std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts;
    std::unordered_map<ResourceId, std::unique_ptr<Image>> images;
    std::unordered_map<ResourceId, std::unique_ptr<Shader>> shaders;
};

#endif
