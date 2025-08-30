#include "SwapChain.h"
#include "Device.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include "../third-party/spdlog/include/spdlog/fmt/fmt.h"
#include <vulkan/vulkan.h>

SwapChain::SwapChain(
	SwapChainSupportDetails swapChainSupportDetails,
	VkSurfaceFormatKHR chosenSurfaceFormat,
	Device &device,
	VkSurfaceKHR surface,
	uint32_t framebufferWdt, 
	uint32_t framebufferHgt
) 
: swapChainSupportDetails(swapChainSupportDetails), 
	surfaceFormat(chosenSurfaceFormat), 
	device(device.getDeviceHandle()), 
	surface(surface)
{
    createSwapChain(
		device.getQueueFamilyIndices().graphics.value(), 
		device.getQueueFamilyIndices().present.value(), 
		framebufferWdt, 
		framebufferHgt
	);
	createImageViews();
}

SwapChain::~SwapChain()
{
	for (auto &imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

VkResult SwapChain::acquireNextImage(uint32_t *imageIndex, VkSemaphore semaphore, VkFence fence)
{
	return vkAcquireNextImageKHR(
		device, 
		swapChain, 
		UINT64_MAX, 
		semaphore, 
		fence, 
		imageIndex
	);
}

VkResult SwapChain::queuePresent(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore semaphore)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &semaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	return vkQueuePresentKHR(presentQueue, &presentInfo);
}

const VkSwapchainKHR &SwapChain::getHandle() const
{
    return swapChain;
}

const SwapChainSupportDetails &SwapChain::getSwapChainSupportDetails() const
{
	return swapChainSupportDetails;
}

const VkSurfaceFormatKHR &SwapChain::getSurfaceFormat() const
{
    return surfaceFormat;
}

size_t SwapChain::getImageCount() const
{
    return swapChainImages.size();
}

VkExtent2D SwapChain::getExtent() const
{
    return swapChainExtent;
}

const std::vector<VkImageView> &SwapChain::getImageViews() const
{
	return swapChainImageViews;
}

SwapChainSupportDetails SwapChain::querySwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails swapChainSupportDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainSupportDetails.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapChainSupportDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainSupportDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		swapChainSupportDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapChainSupportDetails.presentModes.data());
	}

	return swapChainSupportDetails;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t framebufferWdt, uint32_t framebufferHgt)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent{
            std::clamp(framebufferWdt, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(framebufferHgt, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };

        return actualExtent;
    }
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

void SwapChain::createSwapChain(
    uint32_t graphicsQueueIdx, 
    uint32_t presentQueueIdx,
    uint32_t framebufferWdt,
    uint32_t framebufferHgt
) {
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.capabilities, framebufferWdt, framebufferHgt);

	uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount;
	if (swapChainSupportDetails.capabilities.maxImageCount > 0 && 
		imageCount > swapChainSupportDetails.capabilities.maxImageCount
	) {
		imageCount = swapChainSupportDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// if the present and graphics queues are seperate, we must explicitly share their images
	uint32_t queueFamilyIndices[] = {graphicsQueueIdx, presentQueueIdx};
	if(graphicsQueueIdx != presentQueueIdx) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateSwapchainKHR failed with code {}", (int32_t) result));
	}

	// get and store the handles to the swap chain images (there may be more than requested)
	result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkGetSwapchainImagesKHR failed with code {}", (int32_t) result));
	}
	swapChainImages.assign(imageCount, VK_NULL_HANDLE);
	result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkGetSwapchainImagesKHR failed with code {}", (int32_t) result));
	}
	swapChainExtent = extent;
}

void SwapChain::createImageViews()
{
    swapChainImageViews.assign(swapChainImages.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = surfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error(fmt::format("vkCreateImageView failed with code {}", (int32_t) result));
		}
	}
}


