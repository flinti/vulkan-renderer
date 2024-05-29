#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "GraphicsPipeline.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "Mesh.h"

#include <cstddef>
#include <memory>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include <vector>
#include <vulkan/vulkan_core.h>


class Application
{
public:
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        bool isComplete() const
        {
            return graphics.has_value() && present.has_value();
        }
    };

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 800;

	Application(spdlog::logger &log, bool enableValidationLayers, uint32_t concurrentFrames);
	~Application();
	void run();
    void setTargetFps(float targetFps);
private:
    void initWindow();
    void initVulkan();
    void createInstance();
    void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void setupDebugMessenger();
    void createSurface();
    bool checkValidationLayersSupported();
    void findAndChooseDevice();
    bool isDeviceSuitable(
        VkPhysicalDevice device, 
        const QueueFamilyIndices &queueFamilyIndices, 
        const SwapChainSupportDetails &swapChainSupportDetails, 
        const VkPhysicalDeviceProperties &deviceProperties, 
        const VkPhysicalDeviceFeatures &deviceFeatures
    );
    bool checkDeviceRequiredExtensionsSupport(VkPhysicalDevice device);
    QueueFamilyIndices findNeededQueueFamilyIndices(VkPhysicalDevice device);
    void createLogicalDevice();
    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    void createRenderPassAndSwapChain();
    void createSwapChain(const SwapChainSupportDetails &swapChainSupportDetails, const VkSurfaceFormatKHR &chosenSurfaceFormat);
    void recreateSwapChain();
    void createCommandPools();
    void createCommandBuffers();
    void createSyncObjects();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void mainLoop();
    void updateInfoDisplay();
    void draw();
    void cleanup();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
    static void framebufferResized(GLFWwindow* window, int width, int height);

	spdlog::logger &log;

    uint32_t concurrentFrames;
    GLFWwindow *window = nullptr;
    bool paused = false;
    decltype(std::chrono::high_resolution_clock::now()) startedAtTimePoint;
    float targetFps = 60.f;
    float frameRate = 0.f;
    float secondsRunning = 0.f;
    uint64_t frameCounter = 0;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilyIndices;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<GraphicsPipeline> graphicsPipeline;
    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    uint32_t currentFrameIndex = 0;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandPool transferCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> frameFences;
    bool needsSwapChainRecreation = false;
    Mesh testingMesh;

    std::vector<VkExtensionProperties> extensions;
    bool isValidationLayersEnabled = false;
    std::vector<const char *> requiredValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };
    std::vector<const char *> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};






#endif
