#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "Camera.h"
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
#include "Resource.h"

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/scalar_uint_sized.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>


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
    void recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, Frame &frame);
    void mainLoop();
    void updateInfoDisplay();
    void updateCamera();
    void handleInput();
    void draw();
    void cleanupSwapChainAndFramebuffers();
    void cleanup();
    void addMaterial(std::unique_ptr<Material> material);
    std::pair<uint32_t, Material *> addMaterial(const MaterialResource &resource);
    uint32_t addObject(
        const MeshResource &mesh,
        const Material &material,
        glm::mat4 transform = glm::mat4{1.f},
        std::string name = ""
    );
    uint32_t addObject(
        const MeshResource &mesh,
        glm::mat4 transform = glm::mat4{1.f},
        std::string name = ""
    );
    RenderObject &getObject(uint32_t id);
    void removeObject(uint32_t id);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT, 
        VkDebugUtilsMessageTypeFlagsEXT, 
        const VkDebugUtilsMessengerCallbackDataEXT*, 
        void *
    );
    static void onFramebufferResized(GLFWwindow* window, int width, int height);
    static void onScrolled(GLFWwindow* window, double xoffset, double yoffset);
    static void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods);

    uint32_t concurrentFrames;
    GLFWwindow *window = nullptr;
    bool paused = false;
    bool exited = false;
    decltype(std::chrono::high_resolution_clock::now()) startedAtTimePoint;
    float targetFps = 60.f;
    float frameRate = 0.f;
    float secondsRunning = 0.f;
    uint64_t frameCounter = 0;
    double scrollY = 0.0;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    Camera camera;
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
    uint32_t nextMaterialId = 1;
    std::unordered_map<uint32_t, std::unique_ptr<GraphicsPipeline>> graphicsPipelines;
    uint32_t nextId = 1;
    std::vector<RenderObject> renderObjects;
    std::unordered_map<uint32_t, size_t> renderObjectIdIndexMap;
};






#endif
