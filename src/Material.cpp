#include "Material.h"
#include "Device.h"
#include "DeviceAllocator.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "VkHelpers.h"
#include <cstdint>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <utility>
#include <vulkan/vulkan.h>


Material::Material(uint32_t id, Device &device, const MaterialResource &resource)
    : id(id),
    name(resource.getData().name),
    device(device),
    vertexShader(*resource.getData().vertexShader),
    fragmentShader(*resource.getData().fragmentShader),
    images(createImages(resource)),
    imageViews(createImageViews()),
    sampler(requestSampler()),
    parameterBuffer(createParameterBuffer(resource)),
    descriptorSetLayoutBindings(createDescriptorSetLayoutBindings()),
    descriptorImageInfos(createDescriptorImageInfos()),
    descriptorBufferInfos(createDescriptorBufferInfos())
{
    spdlog::info("Material {}({}): created", id, name);
}

Material::Material(Material &&other)
    : id(other.id),
    name(std::move(other.name)),
    device(other.device),
    vertexShader(other.vertexShader),
    fragmentShader(other.fragmentShader),
    images(std::move(other.images)),
    imageViews(std::move(other.imageViews)),
    sampler(other.sampler),
    parameterBuffer(std::move(other.parameterBuffer)),
    descriptorSetLayoutBindings(std::move(other.descriptorSetLayoutBindings)),
    descriptorImageInfos(std::move(other.descriptorImageInfos)),
    descriptorBufferInfos(std::move(other.descriptorBufferInfos))
{
    other.sampler = VK_NULL_HANDLE;
}

Material::~Material()
{
    images.clear();
    for (auto &imageView : imageViews) {
        vkDestroyImageView(device.getDeviceHandle(), imageView, nullptr);
    }
}


uint32_t Material::getId() const
{
    return id;
}

const ShaderResource &Material::getVertexShaderResource() const
{
    return vertexShader;
}

const ShaderResource &Material::getFragmentShaderResource() const
{
    return fragmentShader;
}

VkSampler Material::getSamplerHandle() const
{
    return sampler;
}

const std::vector<VkDescriptorSetLayoutBinding> &Material::getDescriptorSetLayoutBindings() const
{
    return descriptorSetLayoutBindings;
}

const std::map<uint32_t, VkDescriptorImageInfo> &Material::getDescriptorImageInfos() const
{
    return descriptorImageInfos;
}


const std::map<uint32_t, VkDescriptorBufferInfo> &Material::getDescriptorBufferInfos() const
{
    return descriptorBufferInfos;
}


std::vector<Image *> Material::createImages(const std::vector<const ImageResource *> &imageResources)
{
    spdlog::info("Material {}({}): creating images", id, name);

    std::vector<Image *> images;
    images.reserve(imageResources.size());

    for (const ImageResource *imageResource : imageResources) {
        images.emplace_back(&device.getObjectCache().getImage(*imageResource));
    }

    spdlog::info("Material {}({}): created {} images", id, name, images.size());

    return images;
}

std::vector<Image *> Material::createImages(const MaterialResource &resource)
{
    spdlog::info("Material {}({}): creating images from MaterialResource", id, name);
    // TODO: the images for a single ImageResource should not be created multiple times
    const auto &resourceData = resource.getData();
    std::vector<const ImageResource *> imageResources;
    imageResources.reserve(4);
    if (resourceData.ambientTexture) {
        imageResources.push_back(resourceData.ambientTexture);
    }
    if (resourceData.diffuseTexture) {
        imageResources.push_back(resourceData.diffuseTexture);
    }
    if (resourceData.specularTexture) {
        imageResources.push_back(resourceData.specularTexture);
    }
    if (resourceData.normalTexture) {
        imageResources.push_back(resourceData.normalTexture);
    }
    return createImages(imageResources);
}

std::vector<VkImageView> Material::createImageViews()
{
    size_t imageCount = images.size();
    spdlog::info("Material {}({}): creating {} image views", id, name, imageCount);

    std::vector<VkImageView> imageViews(imageCount, VK_NULL_HANDLE);

    for (size_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i]->getImageHandle();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(
            device.getDeviceHandle(),
            &viewInfo,
            nullptr,
            &imageViews[i]
        ));
    }
    
    return imageViews;
}

VkSampler Material::requestSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    return device.getObjectCache().getSampler(samplerInfo);
}

std::vector<VkDescriptorSetLayoutBinding> Material::createDescriptorSetLayoutBindings()
{
    size_t bindingsCount = images.size() + 1;
    spdlog::info("Material {}({}): creating {} descriptor set layout bindings", id, name, bindingsCount);

    std::vector<VkDescriptorSetLayoutBinding> bindings(bindingsCount, VkDescriptorSetLayoutBinding{});

    bindings[0] = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    for (size_t i = 1; i < bindings.size(); ++i) {
        VkDescriptorSetLayoutBinding &samplerDescriptorSetLayoutBinding = bindings[i];
        samplerDescriptorSetLayoutBinding.binding = i;
        samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorSetLayoutBinding.descriptorCount = 1;
        samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

	return bindings;
}

std::map<uint32_t, VkDescriptorImageInfo> Material::createDescriptorImageInfos()
{
    size_t imageCount = images.size();
    spdlog::info("Material {}({}): creating {} image binding infos", id, name, imageCount);

    std::map<uint32_t, VkDescriptorImageInfo> imageBindingInfos;

    for (size_t i = 0; i < imageCount; ++i) {
        imageBindingInfos[i + 1] = VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = imageViews[i],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    return imageBindingInfos;
}


std::map<uint32_t, VkDescriptorBufferInfo> Material::createDescriptorBufferInfos()
{
    return std::map<uint32_t, VkDescriptorBufferInfo>{
        std::make_pair(
            0,
            VkDescriptorBufferInfo{
                .buffer = parameterBuffer.getHandle(),
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            }
        )
    };
}

Buffer Material::createParameterBuffer(const Parameters &params)
{
    return Buffer{
        device.getAllocator(),
        (void *) &params,
        sizeof(params),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
}

Buffer Material::createParameterBuffer(const MaterialResource &resource)
{
    const auto &resourceData = resource.getData();

    Parameters params{
        .ambient = resourceData.ambient,
        .diffuse = resourceData.diffuse,
        .specularAndShininess = glm::vec4(resourceData.specular, resourceData.shininess),
    };
    return createParameterBuffer(params);
}
