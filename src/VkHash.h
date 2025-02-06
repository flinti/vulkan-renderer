#ifndef VKHASH_H_
#define VKHASH_H_

#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "Utility.h"

#include <vulkan/vulkan.h>
#include <unordered_map>


namespace std
{
    template <>
    struct hash<VkSamplerCreateInfo>
    {

        std::size_t operator()(const VkSamplerCreateInfo &info) const
        {
            using namespace Utility;

            std::size_t result = 0;

            hash_combine(result, info.pNext);
            hash_combine(result, info.flags);
            hash_combine(result, info.magFilter);
            hash_combine(result, info.minFilter);
            hash_combine(result, info.mipmapMode);
            hash_combine(result, info.addressModeU);
            hash_combine(result, info.addressModeV);
            hash_combine(result, info.addressModeW);
            hash_combine(result, info.mipLodBias);
            hash_combine(result, info.anisotropyEnable);
            hash_combine(result, info.maxAnisotropy);
            hash_combine(result, info.compareEnable);
            hash_combine(result, info.compareOp);
            hash_combine(result, info.minLod);
            hash_combine(result, info.maxLod);
            hash_combine(result, info.borderColor);
            hash_combine(result, info.unnormalizedCoordinates);

            return result;
        }
    };

    template <>
    struct hash<VkDescriptorBufferInfo>
    {
        std::size_t operator()(const VkDescriptorBufferInfo &info) const
        {
            using namespace Utility;

            std::size_t result = 0;

            hash_combine(result, info.buffer);
            hash_combine(result, info.offset);
            hash_combine(result, info.range);

            return result;
        }
    };

    template <>
    struct hash<VkDescriptorImageInfo>
    {
        std::size_t operator()(const VkDescriptorImageInfo &info) const
        {
            using namespace Utility;
            
            std::size_t result = 0;

            hash_combine(result, info.sampler);
            hash_combine(result, info.imageView);
            hash_combine(result, info.imageLayout);

            return result;
        }
    };

    template <>
    struct hash<VkDescriptorSetLayoutBinding>
    {
        std::size_t operator()(const VkDescriptorSetLayoutBinding &object) const
        {
            using namespace Utility;
            
            std::size_t result = 0;

            hash_combine(result, object.binding);
            hash_combine(result, object.descriptorType);
            hash_combine(result, object.descriptorCount);
            hash_combine(result, object.stageFlags);
            if (object.pImmutableSamplers != nullptr) {
                for (size_t i = 0; i < object.descriptorCount; ++i) {
                    hash_combine(result, object.pImmutableSamplers[i]);
                }
            }

            return result;
        }
    };

    template <>
    struct hash<DescriptorSetLayout>
    {
        std::size_t operator()(const DescriptorSetLayout &object) const
        {
            using namespace Utility;
            
            std::size_t result = 0;

            hash_combine(result, object.getHandle());
            for (const auto &binding : object.getBindings()) {
                hash_combine(result, binding);
            }

            return result;
        }
    };

}

#endif
