#ifndef SWAP_CHAIN_H_
#define SWAP_CHAIN_H_

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class RenderPass;
class Device;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain
{
public:
    SwapChain(
        SwapChainSupportDetails swapChainSupportDetails,
        VkSurfaceFormatKHR chosenSurfaceFormat,
        Device &device,
        VkSurfaceKHR surface,
        uint32_t framebufferWdt, 
        uint32_t framebufferHgt
    );
    ~SwapChain();

    VkResult acquireNextImage(uint32_t *imageIndex, VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);
    VkResult queuePresent(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore semaphore);

    const VkSwapchainKHR &getHandle() const;
    const SwapChainSupportDetails &getSwapChainSupportDetails() const;
    const VkSurfaceFormatKHR &getSurfaceFormat() const;
    size_t getImageCount() const;
    VkExtent2D getExtent() const;
    const std::vector<VkImageView> &getImageViews() const;

    static SwapChainSupportDetails querySwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
private:
    void cleanupSwapChain();
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t framebufferWdt, uint32_t framebufferHgt);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

    void createSwapChain(
        uint32_t graphicsQueueIdx, 
        uint32_t presentQueueIdx,
        uint32_t framebufferWdt,
        uint32_t framebufferHgt
    );
    void createImageViews();
    void createFramebuffers();

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkExtent2D swapChainExtent{};
    
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surfaceFormat{};
    SwapChainSupportDetails swapChainSupportDetails{};
};

#endif
