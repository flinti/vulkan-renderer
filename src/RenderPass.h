#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

#include <vulkan/vulkan.h>

class RenderPass
{
public:
    RenderPass(VkDevice device, VkFormat imageFormat);
    ~RenderPass();

    VkRenderPass getHandle() const;

private:
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDevice device;
    VkFormat imageFormat;
};

#endif
