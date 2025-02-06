#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <vulkan/vulkan.h>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vector>

class Instance
{
public:
    Instance(
        std::vector<const char *> extensionsToEnable = { VK_KHR_SURFACE_EXTENSION_NAME },
        bool enableValidationLayers = false
    );
    ~Instance();

    VkInstance getHandle();
    bool hasValidationLayersEnabled() const;
    const std::vector<const char *> &getValidationLayers() const;
private:
    void createInstance();
    void setupDebugMessenger();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    bool checkValidationLayersSupported();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );

    bool isValidationLayersEnabled = false;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    std::vector<const char *> extensionsToEnable;

    std::vector<VkExtensionProperties> extensions;
    std::vector<const char *> requiredValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
};

#endif
