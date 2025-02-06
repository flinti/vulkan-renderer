#ifndef SHADER_H_
#define SHADER_H_

#include "Resource.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class Device;

class Shader
{
public:
    typedef std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> DescriptorSetLayoutBindingMap;

    Shader(
        Device &device,
        const ShaderResource &resource
    );
    Shader(const Shader &) = delete;
    Shader(Shader &&other);
    ~Shader();

    const ShaderResource &getResource() const;
    const DescriptorSetLayoutBindingMap &getDescriptorSetLayoutBindingMap() const;
    bool hasSet(uint32_t set) const;
    const std::vector<VkDescriptorSetLayoutBinding> &getDescriptorSetLayoutBindings(uint32_t set) const;
    VkShaderModule getShaderModule() const;

    std::string listBindings() const;
private:
    VkShaderModule createShaderModule();

    Device &device;
    const ShaderResource &resource;
    VkShaderModule shaderModule;
};

#endif
