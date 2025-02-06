#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "Buffer.h"
#include "DeviceAllocator.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "Resource.h"
#include "Shader.h"

#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <vulkan/vulkan.h>

class Device;

class Material
{
public:
    struct Parameters
    {
        glm::vec3 ambient;
        float pad1;
        glm::vec3 diffuse;
        float pad2;
        glm::vec4 specularAndShininess;
    };

    Material(
        uint32_t id,
        Device &device,
        const MaterialResource &resource
    );
    Material(const Material &) = delete;
    Material(Material &&other);
    ~Material();

    uint32_t getId() const;
    const ShaderResource &getVertexShaderResource() const;
    const ShaderResource &getFragmentShaderResource() const;
    VkSampler getSamplerHandle() const;
    const std::vector<VkDescriptorSetLayoutBinding> &getDescriptorSetLayoutBindings() const;
    const std::map<uint32_t, VkDescriptorImageInfo> &getDescriptorImageInfos() const;
    const std::map<uint32_t, VkDescriptorBufferInfo> &getDescriptorBufferInfos() const;
private:
    std::vector<Image *> createImages(const std::vector<const ImageResource *> &imageResources);
    std::vector<Image *> createImages(const MaterialResource &resource);
    std::vector<VkImageView>  createImageViews();
    VkSampler requestSampler();
    std::vector<VkDescriptorSetLayoutBinding> createDescriptorSetLayoutBindings();
    std::map<uint32_t, VkDescriptorImageInfo> createDescriptorImageInfos();
    std::map<uint32_t, VkDescriptorBufferInfo> createDescriptorBufferInfos();
    Buffer createParameterBuffer(const Parameters &params);
    Buffer createParameterBuffer(const MaterialResource &resource);

    uint32_t id;
    std::string name;
    Device &device;
    const ShaderResource &vertexShader;
    const ShaderResource &fragmentShader;
    std::vector<Image *> images;
    std::vector<VkImageView> imageViews;
    VkSampler sampler = VK_NULL_HANDLE;
    Buffer parameterBuffer;
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    std::map<uint32_t, VkDescriptorImageInfo> descriptorImageInfos;
    std::map<uint32_t, VkDescriptorBufferInfo> descriptorBufferInfos;
};

#endif
