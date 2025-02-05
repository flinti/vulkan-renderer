#include "Image.h"
#include "Device.h"
#include "ResourceRepository.h"
#include "VkHelpers.h"
#include "../third-party/stb/stb_image.h"

#include <filesystem>
#include <spdlog/fmt/fmt.h>
#include <stdexcept>
#include <utility>
#include <vulkan/vulkan_core.h>

Image::Image(Device &device, const std::filesystem::path &image)
    : device(device),
    image(createImage(image))
{
}

Image::Image(Device &device, const ImageResource &image)
    : device(device),
    image(createImage(image))
{
}

Image::Image(Image &&i)
    : device(i.device),
    image(i.image)
{
    i.image = std::make_pair<VkImage, VmaAllocation>(VK_NULL_HANDLE, VK_NULL_HANDLE);
}

Image::~Image()
{
    if (image.first != VK_NULL_HANDLE && image.second != VK_NULL_HANDLE) {
        device.getAllocator().free(image);
    }
}

VkImage Image::getImageHandle() const
{
    return image.first;
}

std::pair<VkImage, VmaAllocation> Image::createImage(const std::filesystem::path &imagePath)
{
    int wdt;
    int hgt;
    int channels;
    std::unique_ptr<unsigned char, void(*)(unsigned char *)> imageData(stbi_load(
        imagePath.c_str(), 
        &wdt, 
        &hgt, 
        &channels, 
        STBI_rgb_alpha
    ), [](unsigned char *data) {
        stbi_image_free(static_cast<void *>(data));
    });

    if (!imageData) {
        throw std::runtime_error(fmt::format("Image::createImage: failed to load image {}", imagePath.c_str()));
    }

    image = device.getAllocator().allocateDeviceLocalImageAndTransfer(
        static_cast<void *>(imageData.get()), 
        wdt, 
        hgt, 
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );

    return image;
}

std::pair<VkImage, VmaAllocation> Image::createImage(const ImageResource &image)
{
    const auto &resourceData = image.getData();
    auto createdImage = device.getAllocator().allocateDeviceLocalImageAndTransfer(
        resourceData.data, 
        resourceData.width, 
        resourceData.height, 
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT
    );
    return createdImage;
}
