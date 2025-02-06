#ifndef IMAGE_H_
#define IMAGE_H_

#include "DeviceAllocator.h"
#include "Resource.h"

#include <utility>
#include <vulkan/vulkan.h>
#include <filesystem>

class DeviceAllocator;

class Image
{
public:
    Image(Device &device, const std::filesystem::path &image);
    Image(Device &device, const ImageResource &image);
    Image(const Image &) = delete;
    Image(Image &&);
    ~Image();

    VkImage getImageHandle() const;
private:
    std::pair<VkImage, VmaAllocation> createImage(const std::filesystem::path &image);
    std::pair<VkImage, VmaAllocation> createImage(const ImageResource &image);

    Device &device;

    std::pair<VkImage, VmaAllocation> image = std::make_pair<VkImage, VmaAllocation>(VK_NULL_HANDLE, VK_NULL_HANDLE);
};

#endif
