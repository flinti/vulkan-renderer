#include "Application.h"
#include "GraphicsPipeline.h"
#include "Material.h"
#include "Mesh.h"
#include "RenderObject.h"
#include "RenderPass.h"
#include "ResourceRepository.h"
#include "VkHelpers.h"

#include <array>
#include <chrono>
#include <cmath>
#include <fmt/core.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <sstream>


Application::Application(bool enableValidationLayers, uint32_t concurrentFrames, bool singleFrame)
	: concurrentFrames(concurrentFrames),
	exited(singleFrame)
{
	initWindow();
	initVulkan(enableValidationLayers);
}

Application::~Application()
{
	spdlog::info("running application destructor...");
	cleanup();
}


void Application::run()
{
	startedAtTimePoint = std::chrono::high_resolution_clock::now();
	mainLoop();
}


void Application::setTargetFps(float targetFps)
{
	this->targetFps = targetFps;
}

void Application::initWindow()
{
	spdlog::info("initializing window...");
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Demo", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, onFramebufferResized);
	glfwSetScrollCallback(window, onScrolled);
	glfwSetKeyCallback(window, onKeyEvent);
}

void Application::initVulkan(bool validationLayers)
{
	spdlog::info("initializing vulkan...");

	// request the required extensions
	uint32_t glfwRequiredExtensionsCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionsCount);
	if (glfwExtensions == nullptr) {
		throw std::runtime_error("glfwGetRequiredInstanceExtensions returned NULL");
	}
	std::vector<const char *> instanceExtensions(glfwExtensions, glfwExtensions + glfwRequiredExtensionsCount);

	instance = std::make_unique<Instance>(instanceExtensions, validationLayers);
	
	VK_ASSERT(glfwCreateWindowSurface(instance->getHandle(), window, nullptr, &surface));
	
	device = std::make_unique<Device>(
		*instance, 
		surface,
		std::vector<const char *>{ 
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		}
	);

	createRenderPassAndSwapChain();

	loadResources();

	createInitialObjects();
	
	frames.reserve(concurrentFrames);
	for (size_t i = 0; i < concurrentFrames; ++i) {
		Frame &frame = frames.emplace_back(
			*device, 
			device->getQueueFamilyIndices().graphics.value()
		);
		updateDescriptors(frame);
	}
}

VkSurfaceFormatKHR Application::chooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	if(availableFormats.empty()) {
		throw std::runtime_error("chooseSwapSurfaceFormat: availableFormats must not be empty");
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB 
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

void Application::createRenderPassAndSwapChain()
{
	spdlog::info("creating swap chain...");

	auto swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(
		device->getPhysicalDeviceHandle(), surface
	);
	auto chosenSurfaceFormat = chooseSwapChainSurfaceFormat(
		swapChainSupportDetails.formats
	);

	std::string surfaceFormats;
	for (const auto &i : swapChainSupportDetails.formats) {
		surfaceFormats += fmt::format(
			"\t{} ({})\n",
			string_VkFormat(i.format),
			string_VkColorSpaceKHR(i.colorSpace)
		);
	}
	spdlog::info("Supported swap chain surface formats:\n{}", surfaceFormats);

	renderPass = std::make_unique<RenderPass>(
		device->getDeviceHandle(), 
		chosenSurfaceFormat.format
	);
	createSwapChainAndFramebuffers(swapChainSupportDetails, chosenSurfaceFormat);
}

void Application::createFramebuffers(uint32_t width, uint32_t height)
{
	auto swapChainImageViews = swapChain->getImageViews();
	swapChainFramebuffers.assign(swapChainImageViews.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImage->getImageViewHandle(),
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass->getHandle();
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;

		VK_ASSERT(vkCreateFramebuffer(
			device->getDeviceHandle(), 
			&framebufferInfo,
			 nullptr, 
			 &swapChainFramebuffers[i]
		));
	}
}

void Application::createSwapChainAndFramebuffers(const SwapChainSupportDetails &swapChainSupportDetails, const VkSurfaceFormatKHR &chosenSurfaceFormat)
{
	int wdt = 0;
	int hgt = 0;
	glfwGetFramebufferSize(window, &wdt, &hgt);
	
	swapChain = std::make_unique<SwapChain>(
		swapChainSupportDetails,
		chosenSurfaceFormat,
		*device,
		surface,
		static_cast<uint32_t>(wdt), 
		static_cast<uint32_t>(hgt)
	);
	spdlog::info("Created swap chain with {} images", swapChain->getImageCount());
	depthImage = std::make_unique<DepthImage>(
		*device, 
		static_cast<uint32_t>(wdt), 
		static_cast<uint32_t>(hgt)
	);
	createFramebuffers(static_cast<uint32_t>(wdt), static_cast<uint32_t>(hgt));
}

void Application::recreateSwapChain() 
{
	device->waitDeviceIdle();

	SwapChainSupportDetails swapChainSupportDetails = SwapChain::querySwapChainSupportDetails(
		device->getPhysicalDeviceHandle(), 
		surface
	);
	VkSurfaceFormatKHR chosenSurfaceFormat = swapChain->getSurfaceFormat();

	cleanupSwapChainAndFramebuffers();

    createSwapChainAndFramebuffers(swapChainSupportDetails, chosenSurfaceFormat);
}

void Application::loadResources()
{
	spdlog::info("creating resource repository and loading resources...");

	resourceRepository = std::make_unique<ResourceRepository>("image/default");

	spdlog::info("loaded resources:\n{}", resourceRepository->resourceTree(1));
}

void Application::createInitialObjects()
{
	spdlog::info("creating initial objects...");

	spdlog::info("add cone...");
	addObject(
		resourceRepository->getMesh("mesh/cone"),
		glm::mat4{1.f},
		"mid"
	);
	spdlog::info("add cylinder...");
	addObject(
		resourceRepository->getMesh("mesh/cylinder"),
		glm::translate(
			glm::rotate(glm::mat4{1.f}, glm::quarter_pi<float>(), glm::vec3(0.f, 0.f, 1.f)),
			 glm::vec3(-1.5f, -2.5f, 0.f)
		),
		"other"
	);
	spdlog::info("add sun...");
	addObject(
		resourceRepository->getMesh("mesh/icosphere"),
		glm::translate(glm::mat4(1.f), glm::vec3(5.f, 5.f, 3.f)),
		"sun"
	);

	camera.lookAt(glm::vec3(0.f), 10.f, glm::quarter_pi<float>(), glm::quarter_pi<float>());
}

void Application::updateDescriptors(Frame &frame)
{
	// make sure all required descriptor sets have been allocated and initialize them
	for (auto &pipelineIter : graphicsPipelines) {
		GraphicsPipeline &pipeline = *pipelineIter.second;

		frame.getDescriptorSet(
			0, 
			pipeline.getMaterialDescriptorSetLayout(), 
			pipeline.getMaterial().getDescriptorBufferInfos(),
			pipeline.getMaterial().getDescriptorImageInfos()
		);
	}
	frame.updateDescriptorSets(0);
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, Frame &frame)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkExtent2D swapChainExtent = swapChain->getExtent();
	float vpWdt = static_cast<float>(swapChainExtent.width);
	float vpHgt = static_cast<float>(swapChainExtent.height);

	// begin render pass
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {0.f, 0.f, 0.f, 1.f};
	clearValues[1].depthStencil = { 1.f, 0 };
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->getHandle();
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = vpWdt;
	viewport.height = vpHgt;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (const auto &r : renderObjects) {
		GraphicsPipeline &pipeline = *graphicsPipelines.at(r.getMaterial().getId());
		pipeline.bind(commandBuffer);

		pipeline.bindDescriptorSet(
			commandBuffer,
			DescriptorSetIndex::GLOBAL_UNIFORM_DATA,
			frame.getGlobalUniformDataDescriptorSet()
		);
		pipeline.bindDescriptorSet(commandBuffer, DescriptorSetIndex::MATERIAL_DATA,
			frame.getDescriptorSet(
				0,
				pipeline.getMaterialDescriptorSetLayout(), 
				r.getMaterial().getDescriptorBufferInfos(),
				r.getMaterial().getDescriptorImageInfos()
		));

		pushConstants.transform = r.getTransform();
		pushConstants.normalTransform = glm::transpose(glm::inverse(pushConstants.transform));
		pipeline.pushConstants(
			commandBuffer, 
			static_cast<void *>(&pushConstants), 
			sizeof(pushConstants)
		);
		
		r.enqueueDrawCommands(commandBuffer);
	}

	vkCmdEndRenderPass(commandBuffer);

	VK_ASSERT(vkEndCommandBuffer(commandBuffer));
}

void Application::updateCamera()
{
	VkExtent2D swapChainExtent = swapChain->getExtent();
	float vpWdt = static_cast<float>(swapChainExtent.width);
	float vpHgt = static_cast<float>(swapChainExtent.height);
	camera.setAspect(vpWdt / vpHgt);
}

void Application::handleInput()
{
	float rotationSensitivity = 0.05f;
	float radialSensitivity = 0.5f;
	float rotationSpeed = rotationSensitivity / frameRate;
	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		radialSensitivity *= 10.f;
		rotationSensitivity *= 10.f;
	}

	if (scrollY != 0.0) {
		camera.addRadius(- radialSensitivity * scrollY);
		scrollY = 0.0;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		camera.drag(-(mouseX - lastMouseX) * rotationSensitivity, -(mouseY - lastMouseY) * rotationSensitivity);
	}
	else {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			camera.drag(0.f, rotationSensitivity);
		}
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			camera.drag(0.f, -rotationSensitivity);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			camera.drag(-rotationSensitivity, 0.f);
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			camera.drag(rotationSensitivity, 0.f);
		}
	}

	lastMouseX = mouseX;
	lastMouseY = mouseY;
}

void Application::draw()
{
	VkFence fence = frames[currentFrameIndex].getFence();
	VkSemaphore imageAvailableSemaphore = frames[currentFrameIndex].getImageAvailableSemaphore();
	VkSemaphore renderFinishedSemaphore = frames[currentFrameIndex].getRenderFinishedSemaphore();
	VkCommandBuffer commandBuffer = frames[currentFrameIndex].getCommandBuffer();
	VkQueue graphicsQueue = device->getGraphicsQueue();
    VkQueue presentQueue = device->getPresentQueue();
	Frame &frame = frames[currentFrameIndex];

	vkWaitForFences(
		device->getDeviceHandle(), 
		1, 
		&fence, 
		VK_TRUE, 
		UINT64_MAX
	);

	updateCamera();

	GlobalUniformData uniformData{};
	uniformData.viewProj = camera.getTransform();
	uniformData.viewPos = camera.getEye();
	uniformData.time = glm::vec4(static_cast<float>(secondsRunning));
	uniformData.lightPosition = glm::vec3(5.f, 5.f, 3.f);
	frame.updateGlobalUniformBuffer(uniformData);

	uint32_t imageIndex;
    VkResult result = swapChain->acquireNextImage(
		&imageIndex, 
		imageAvailableSemaphore
	);
	// needsSwapChainRecreation is handled near the end of this function (deviating from the tutorial) 
	// to avoid that the image available semaphore occasionally remains signalled
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS) {
		spdlog::error("vkAcquireNextImageKHR failed with code {}", (int32_t) result);
	}

	vkResetFences(device->getDeviceHandle(), 1, &fence);

	result = vkResetCommandBuffer(commandBuffer, 0);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkResetCommandBuffer failed with code {}", (int32_t) result));
	}
	recordCommandBuffer(commandBuffer, swapChainFramebuffers[imageIndex], frame);

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkQueueSubmit failed with code {}", (int32_t) result));
	}

	result = swapChain->queuePresent(presentQueue, imageIndex, renderFinishedSemaphore);
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR || needsSwapChainRecreation) {
		needsSwapChainRecreation = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		spdlog::error("vkQueuePresentKHR failed with code {}", (int32_t) result);
	}

	currentFrameIndex = (currentFrameIndex + 1) % concurrentFrames;
}

void Application::mainLoop()
{
	spdlog::info("starting main loop...");

	auto prevFrameTime = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		auto beginFrameTime = std::chrono::high_resolution_clock::now();
		glfwPollEvents();

		++frameCounter;
		secondsRunning = std::chrono::duration<float, std::chrono::seconds::period>(
			std::chrono::high_resolution_clock::now() - startedAtTimePoint
		).count();

		if (!paused) {
			handleInput();
			draw();
		}

		auto endFrameTime = std::chrono::high_resolution_clock::now();
		
		// limit frame rate
		float frameDuration = std::chrono::duration<float, std::chrono::seconds::period>(
			endFrameTime - beginFrameTime
		).count();

		float targetFrameDuration = 1.f / targetFps;
		float sleepSec = targetFrameDuration - frameDuration;
		if (sleepSec > 0.f) {
			std::this_thread::sleep_for(
				std::chrono::duration<float, std::chrono::seconds::period>(sleepSec)
			);
		}

		// measure frame rate
		frameDuration = std::chrono::duration<float, std::chrono::seconds::period>(
			endFrameTime - prevFrameTime
		).count();

		prevFrameTime = endFrameTime;
		frameRate = 1.f / frameDuration;
		updateInfoDisplay();

		if (exited) {
			spdlog::info("application exiting gracefully");
			break;
		}
	}

	device->waitDeviceIdle();
}

void Application::updateInfoDisplay()
{
	std::stringstream s;
	s << "Demo " << std::setprecision(3) << frameRate << " fps";
	if (paused) {
		s << " paused";
	}
	glfwSetWindowTitle(window, s.str().c_str());
}

void Application::cleanupSwapChainAndFramebuffers()
{
	for (auto &framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device->getDeviceHandle(), framebuffer, nullptr);
    }
	swapChain.reset();
}

void Application::cleanup()
{
	spdlog::info("cleaning up...");

	renderObjects.clear();
	frames.clear();
	for (auto &i : graphicsPipelines) {
		i.second.reset();
	}
	for (auto &i : materials) {
		i.second.reset();
	}
	depthImage.reset();
    cleanupSwapChainAndFramebuffers();
	renderPass.reset();
	resourceRepository.reset();
	device.reset();
	vkDestroySurfaceKHR(instance->getHandle(), surface, nullptr);
	instance.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::addMaterial(std::unique_ptr<Material> material)
{
	if (materials.find(material->getId()) != materials.end()) {
		throw std::runtime_error(fmt::format("Material with ID {} already exists", material->getId()));
	}
	Material &inserted = *materials.insert(
		std::make_pair<uint32_t, std::unique_ptr<Material>>(material->getId(), std::move(material))
	).first->second;
	graphicsPipelines.insert(std::make_pair(
		inserted.getId(),
		std::make_unique<GraphicsPipeline>(
			*device,
			*renderPass,
			inserted
	)));
}


std::pair<uint32_t, Material *> Application::addMaterial(const MaterialResource &resource)
{
	uint32_t newId = nextMaterialId++;
	auto mat = std::make_unique<Material>(newId, *device, resource);
	Material *retMat = mat.get();
	addMaterial(std::move(mat));
	return std::make_pair(newId, retMat);
}

uint32_t Application::addObject(
	const MeshResource &mesh,
	const Material &material,
	glm::mat4 transform,
	std::string name
) {
	size_t newIndex = renderObjects.size();
	uint32_t newId = nextId++;

	spdlog::info("add object: id {}, index {}", newId, newIndex);

	renderObjects.emplace_back(newId, *device, mesh, material, name).setTransform(transform);
	renderObjectIdIndexMap.emplace(newId, newIndex);

	return newId;
}


uint32_t Application::addObject(
	const MeshResource &mesh,
	glm::mat4 transform,
	std::string name
) {
	spdlog::info("addObject: mesh {} with material {}", (void*)&mesh, (void*)mesh.getData().getMaterial());
	const MaterialResource *matResource = mesh.getData().getMaterial();
	if (!matResource) {
		throw std::runtime_error("Application::addObject: No material specified and mesh has no assigned material");
	}
	Material *mat = addMaterial(*matResource).second;
	uint32_t id = addObject(mesh, *mat, transform, name);
	return id;
}

RenderObject &Application::getObject(uint32_t id)
{
	const auto indexIter = renderObjectIdIndexMap.find(id);
	if (indexIter != renderObjectIdIndexMap.end()) {
		return renderObjects[indexIter->second];
	}
	else {
		throw std::runtime_error(fmt::format("Object with id {} does not exist!", id));
	}
}

void Application::removeObject(uint32_t id)
{
	const auto indexIter = renderObjectIdIndexMap.find(id);
	if (indexIter != renderObjectIdIndexMap.end()) {
		size_t index = indexIter->second;
		renderObjects.erase(renderObjects.begin() + index);
		renderObjectIdIndexMap.clear();
		for (size_t i = 0; i < renderObjects.size(); ++i) {
			renderObjectIdIndexMap[renderObjects[i].getId()] = i;
		}
	}
}

void Application::onFramebufferResized(GLFWwindow* window, int width, int height)
{
	auto *myThis = static_cast<Application *>(glfwGetWindowUserPointer(window));
	if (width <= 0 || height <= 0) {
		myThis->paused = true;
	}
	else if (myThis->paused == true) {
		myThis->paused = false;
	}
	myThis->needsSwapChainRecreation = true;
}

void Application::onScrolled(GLFWwindow* window, double xoffset, double yoffset)
{
	auto *myThis = static_cast<Application *>(glfwGetWindowUserPointer(window));
	myThis->scrollY = yoffset;
}


void Application::onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto *myThis = static_cast<Application *>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS){
		if (key == GLFW_KEY_P) {
			myThis->paused = !myThis->paused;
		}
		else if (key == GLFW_KEY_KP_ADD) {
			myThis->camera.addFar(10.f);
		}
		else if (key == GLFW_KEY_KP_SUBTRACT) {
			myThis->camera.addFar(-10.f);
		}
	}
}
