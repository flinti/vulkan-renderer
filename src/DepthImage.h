#ifndef DEPTHIMAGE_H_
#define DEPTHIMAGE_H_

#include "DeviceAllocator.h"

#include <vulkan/vulkan.h>

class DepthImage
{
public:
    DepthImage(Device &device, uint32_t width, uint32_t height);
    ~DepthImage();

    VkImage getImageHandle() const;
    VkImageView getImageViewHandle() const;
private:
    void createImage(uint32_t width, uint32_t height);
    void createImageView();

    Device &device;
    std::pair<VkImage, VmaAllocation> depthImage;
    VkImageView depthImageView = VK_NULL_HANDLE;
};

#endif
