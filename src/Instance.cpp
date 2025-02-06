#include "Instance.h"
#include "VkHelpers.h"

#include <vulkan/vulkan.h>
#include <sstream>

Instance::Instance(
    std::vector<const char *> extensionsToEnable,
    bool enableValidationLayers
)
    : extensionsToEnable(extensionsToEnable),
    isValidationLayersEnabled(enableValidationLayers)
{
    createInstance();
    setupDebugMessenger();
}

Instance::~Instance()
{
    if (isValidationLayersEnabled) {
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXT) {
		    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
	}

    vkDestroyInstance(instance, nullptr);
}

VkInstance Instance::getHandle()
{
    return instance;
}

bool Instance::hasValidationLayersEnabled() const
{
    return isValidationLayersEnabled;
}

const std::vector<const char *> &Instance::getValidationLayers() const
{
    return requiredValidationLayers;
}


void Instance::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "None";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// check validation layers & enable if applicable
	if(isValidationLayersEnabled) {
		spdlog::info("Validation layers enabled. Checking layer support...");

		if(!checkValidationLayersSupported()) {
			throw std::runtime_error("The required validation layers are not available!");
		}
		
		createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	} else {
		spdlog::info("Validation layers disabled");
		createInfo.enabledLayerCount = 0;
	}

	// get extension list
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	extensions.resize(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	{
		std::stringstream logLine;
		logLine << "Available Vulkan extensions:";
		for (auto ext : extensions) {
			logLine << "\n\t" << ext.extensionName << " v" << ext.specVersion;
		}
		spdlog::info(logLine.str());
	}

	if(isValidationLayersEnabled) {
		extensionsToEnable.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	createInfo.enabledExtensionCount = extensionsToEnable.size();
	createInfo.ppEnabledExtensionNames = extensionsToEnable.data();

	{
		std::stringstream logLine;
		logLine << "extensions to enable:";
		for (const auto &extension : extensionsToEnable) {
			logLine << "\n\t" << extension;
		}
		spdlog::info(logLine.str());
	}

	// request debug messenger for instance creation and destruction, if applicable
	VkDebugUtilsMessengerCreateInfoEXT instanceCreationDebugMessengerCreateInfo{};
	if (isValidationLayersEnabled) {
		fillDebugMessengerCreateInfo(instanceCreationDebugMessengerCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &instanceCreationDebugMessengerCreateInfo;
	}

	// create vulkan instance
	spdlog::info("creating instance...");
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateInstance failed with code {}", (int32_t)result));
	}
	spdlog::info("Vulkan instance created.");
}

void Instance::setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	fillDebugMessengerCreateInfo(createInfo);

	auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) (vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (vkCreateDebugUtilsMessengerEXT == nullptr) {
		throw std::runtime_error("addres of vkCreateDebugUtilsMessengerEXT could not be looked up!");
	}
	VK_ASSERT(vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
}

void Instance::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = this;
}

bool Instance::checkValidationLayersSupported()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layer : requiredValidationLayers) {
		bool found = false;

		for (const auto &layerProp : availableLayers) {
			if (!strcmp(layer, layerProp.layerName)) {
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




VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
	auto *myThis = static_cast<Instance *>(pUserData);

    spdlog::info("validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}
