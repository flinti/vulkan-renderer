#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "GraphicsPipeline.h"
#include "RenderObject.h"
#include "ResourceRepository.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Buffer.h"
#include "Mesh.h"
#include "Frame.h"
#include "Instance.h"
#include "Device.h"
#include "DeviceAllocator.h"
#include "RenderObject.h"
#include "DepthImage.h"
#include "Image.h"
#include "Material.h"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/scalar_uint_sized.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>


class Application
{
public:
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 800;

	Application(bool enableValidationLayers, uint32_t concurrentFrames, bool singleFrame);
	~Application();
	void run();
    void setTargetFps(float targetFps);
private:
    void initWindow();
    void initVulkan(bool validationLayers);
    void createLogicalDevice();
    VkSurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    void createRenderPassAndSwapChain();
    void createFramebuffers(uint32_t width, uint32_t height);
    void createSwapChainAndFramebuffers(
        const SwapChainSupportDetails &swapChainSupportDetails, 
        const VkSurfaceFormatKHR &chosenSurfaceFormat
    );
    void recreateSwapChain();
    void loadResources();
    void createInitialObjects();
    void updateDescriptors(Frame &frame);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, Frame &frame);
    void mainLoop();
    void updateInfoDisplay();
    void draw();
    void cleanupSwapChainAndFramebuffers();
    void cleanup();
    void addMaterial(std::unique_ptr<Material> material);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
    static void framebufferResized(GLFWwindow* window, int width, int height);

    uint32_t concurrentFrames;
    GLFWwindow *window = nullptr;
    bool paused = false;
    bool exited = false;
    decltype(std::chrono::high_resolution_clock::now()) startedAtTimePoint;
    float targetFps = 60.f;
    float frameRate = 0.f;
    float secondsRunning = 0.f;
    uint64_t frameCounter = 0;
    std::unique_ptr<Instance> instance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::unique_ptr<Device> device;
    std::unique_ptr<ResourceRepository> resourceRepository;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<DepthImage> depthImage;
    std::unique_ptr<RenderPass> renderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    uint32_t currentFrameIndex = 0;
    std::vector<Frame> frames;
    bool needsSwapChainRecreation = false;
    bool recreatingSwapChain = false;
    PushConstants pushConstants;
    std::unordered_map<uint32_t, std::unique_ptr<Material>> materials;
    std::unordered_map<uint32_t, std::unique_ptr<GraphicsPipeline>> graphicsPipelines;
    std::vector<RenderObject> renderObjects;
};






#endif
