#include "Shader.h"

#include "Device.h"
#include "VkHelpers.h"

#include <fmt/core.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

Shader::Shader(
    Device &device,
    const ShaderResource &resource
)
    : device(device),
    resource(resource),
    shaderModule(createShaderModule())
{
}


Shader::Shader(Shader &&other)
    : device(other.device),
    resource(other.resource),
    shaderModule(other.shaderModule)
{
    other.shaderModule = VK_NULL_HANDLE;
}

Shader::~Shader()
{
    if (shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device.getDeviceHandle(), shaderModule, nullptr);
    }
}


const ShaderResource &Shader::getResource() const
{
    return resource;
}

const Shader::DescriptorSetLayoutBindingMap &Shader::getDescriptorSetLayoutBindingMap() const
{
    return resource.getData().bindings;
}


bool Shader::hasSet(uint32_t set) const
{
    return resource.getData().bindings.find(set) != resource.getData().bindings.end();
}

const std::vector<VkDescriptorSetLayoutBinding> &Shader::getDescriptorSetLayoutBindings(uint32_t set) const
{
    auto iter = resource.getData().bindings.find(set);
    if (iter == resource.getData().bindings.end()) {
        throw std::invalid_argument(fmt::format("descriptor set {} does not exist"));
    }
    return iter->second;
}

VkShaderModule Shader::getShaderModule() const
{
    return shaderModule;
}

std::string Shader::listBindings() const
{
    std::stringstream ss;
    for (const auto &i : resource.getData().bindings) {
        for (const auto &binding : i.second) {
            ss << "set " << i.first << " binding " << binding.binding
                << " (" << binding.descriptorCount << "x "
                << string_VkDescriptorType(binding.descriptorType)
                << ")\n";
        }
    }
    return ss.str();
}

VkShaderModule Shader::createShaderModule()
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = resource.getData().code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(resource.getData().code.data());

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VK_ASSERT(vkCreateShaderModule(
        device.getDeviceHandle(),
        &createInfo,
        nullptr,
        &shaderModule
    ));

	return shaderModule;
}
