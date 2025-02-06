#include "Device.h"
#include "DeviceAllocator.h"
#include "VkHelpers.h"
#include "Instance.h"
#include "VulkanObjectCache.h"

#include <iomanip>
#include <ios>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <set>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

Device::Device(
	Instance &instance, 
	VkSurfaceKHR surface,
	std::vector<const char *> extensionsToEnable
)
    : instance(instance),
    surface(surface),
	extensionsToEnable(extensionsToEnable),
	selectedQueueFamilyIndices{},
	graphicsQueue(VK_NULL_HANDLE),
	presentQueue(VK_NULL_HANDLE),
	physicalDevice(chooseSuitablePhysicalDevice()),
	device(createLogicalDevice()),
	transferCommandPool(createTransferCommandPool()),
	allocator(std::make_unique<DeviceAllocator>(
		instance.getHandle(),
		physicalDevice,
		device,
		transferCommandPool,
		graphicsQueue)
	),
	objectCache(std::make_unique<VulkanObjectCache>(*this))
{
}

Device::~Device()
{
	objectCache.reset();
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
	allocator.reset();
	
    vkDestroyDevice(device, nullptr);
}


VulkanObjectCache &Device::getObjectCache()
{
	return *objectCache;
}

DeviceAllocator &Device::getAllocator()
{
	return *allocator;
}


VkInstance Device::getInstanceHandle()
{
    return instance.getHandle();
}

VkPhysicalDevice Device::getPhysicalDeviceHandle()
{
    return physicalDevice;
}

VkDevice Device::getDeviceHandle()
{
    return device;
}

VkQueue Device::getGraphicsQueue()
{
    return graphicsQueue;
}

VkQueue Device::getPresentQueue()
{
    return presentQueue;
}

const QueueFamilyIndices &Device::getQueueFamilyIndices() const
{
    return selectedQueueFamilyIndices;
}

void Device::waitDeviceIdle()
{
    vkDeviceWaitIdle(device);
}


VkPhysicalDevice Device::chooseSuitablePhysicalDevice()
{
	spdlog::info("listing GPUs and choosing suitable ones");

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance.getHandle(), &deviceCount, nullptr);
	if (!deviceCount) {
		throw std::runtime_error("No GPUs found!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance.getHandle(), &deviceCount, devices.data());

	// list devices and select last suitable device
	std::stringstream logLine;
	logLine << "GPUs found:";
	for (auto device : devices) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		QueueFamilyIndices queueFamilyIndices = findNeededQueueFamilyIndices(device);
		SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(device, surface);
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		bool suitable = isDeviceSuitable(device, queueFamilyIndices, swapChainSupportDetails, deviceProperties, deviceFeatures);
		if (suitable) {
			physicalDevice = device;
		}

		logLine << "\n\tvID " << deviceProperties.vendorID 
			<< " dID " << deviceProperties.deviceID << ": " << deviceProperties.deviceName;
	}
	spdlog::info(logLine.str());

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("GPUs were found, but no device is suitable!");
	}

	spdlog::info("suitable device chosen.");
	return physicalDevice;
}

bool Device::isDeviceSuitable(
	VkPhysicalDevice device, 
	const QueueFamilyIndices &queueFamilyIndices,
	const SwapChainSupportDetails &swapChainSupportDetails,
	const VkPhysicalDeviceProperties &deviceProperties, 
	const VkPhysicalDeviceFeatures &deviceFeatures
) {
	bool familyIndicesComplete = queueFamilyIndices.isComplete();
	bool extensionsSupported = checkDeviceRequiredExtensionsSupport(device);
	bool swapChainAdequate = false;
	bool anisotropicFilteringAvailable = deviceFeatures.samplerAnisotropy;
	
	if (extensionsSupported) {
		swapChainAdequate = !swapChainSupportDetails.formats.empty() 
			&& !swapChainSupportDetails.presentModes.empty();
	}

	return familyIndicesComplete && extensionsSupported && swapChainAdequate && anisotropicFilteringAvailable;
}

bool Device::checkDeviceRequiredExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	for (const auto &requiredExtension : extensionsToEnable) {
		bool found = false;
		for (const auto &availableExtension : availableExtensions) {
			if (!strcmp(requiredExtension, availableExtension.extensionName)) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}

	return true;
}

QueueFamilyIndices Device::findNeededQueueFamilyIndices(VkPhysicalDevice device)
{
	QueueFamilyIndices indices{};

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, 
		&queueFamilyCount, 
		nullptr
	);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, 
		&queueFamilyCount, 
		queueFamilies.data()
	);

	uint32_t i = 0;
	std::stringstream logLine;
	logLine << "available queue families:";
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			device, 
			i, surface, 
			&presentSupport
		);

		logLine << "\n\t" 
			<< std::dec << i << ". count " << std::setfill(' ') << std::setw(2) << queueFamily.queueCount 
			<< " 0x" << std::hex << std::setfill('0') << std::setw(8) << queueFamily.queueFlags 
			<< " " << string_VkQueueFlags(queueFamily.queueFlags);
		if (presentSupport) {
			logLine << "|PRESENT";
		}

		if (!indices.isComplete()) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics = i;
			}
			if (presentSupport) {
				indices.present = i;
			}
		}

		i++;
	}
	spdlog::info(logLine.str());

	return indices;
}

VkDevice Device::createLogicalDevice()
{
	spdlog::info("creating logical device...");

	VkDevice device = VK_NULL_HANDLE;

	selectedQueueFamilyIndices = findNeededQueueFamilyIndices(physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueIndices{
		selectedQueueFamilyIndices.graphics.value(), 
		selectedQueueFamilyIndices.present.value()
	};

	float priority = 1.f;
	for (const auto &index : uniqueIndices) {
		VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = index,
			.queueCount = 1,
			.pQueuePriorities = &priority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledExtensionNames = extensionsToEnable.data();
	createInfo.enabledExtensionCount = extensionsToEnable.size();

	{
		std::stringstream logLine;
		logLine << "extensions to enable:";
		for (const auto &extension : extensionsToEnable) {
			logLine << "\n\t" << extension;
		}
		spdlog::info(logLine.str());
	}

	// there is no longer a distinction between device and instance specific layers, but setting those fields for backwards compatibility
	if (instance.hasValidationLayersEnabled()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(instance.getValidationLayers().size());
		createInfo.ppEnabledLayerNames = instance.getValidationLayers().data();
	}

	VK_ASSERT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

	vkGetDeviceQueue(device, selectedQueueFamilyIndices.graphics.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, selectedQueueFamilyIndices.present.value(), 0, &presentQueue);

	spdlog::info("logical device created.");
	return device;
}

VkCommandPool Device::createTransferCommandPool()
{
	spdlog::info("creating transfer command pool...");

	VkCommandPool transferCommandPool;

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = selectedQueueFamilyIndices.graphics.value();

	VK_ASSERT(vkCreateCommandPool(
		device, 
		&poolInfo, 
		nullptr, 
		&transferCommandPool
	));

	return transferCommandPool;
}
