#ifndef GRAPHICS_PIPELINE_H_
#define GRAPHICS_PIPELINE_H_

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "Resource.h"
#include "Shader.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class RenderPass;
class DescriptorSet;
class Material;
class Device;

struct PushConstants {
    glm::mat4 transform;
    glm::mat4 normalTransform;
};


enum class DescriptorSetIndex: uint32_t
{
    GLOBAL_UNIFORM_DATA = 0,
    MATERIAL_DATA = 1,
};

class GraphicsPipeline
{
public:
    GraphicsPipeline(
        Device &device,
        const RenderPass &renderPass,
        const Material &material
    );
    GraphicsPipeline(const GraphicsPipeline &) = delete;
    GraphicsPipeline(GraphicsPipeline &&);
    ~GraphicsPipeline();

    const DescriptorSetLayout &getMaterialDescriptorSetLayout() const; // set = 1
    const Material &getMaterial() const;
    void bind(VkCommandBuffer commandBuffer);
    void bindDescriptorSet(VkCommandBuffer commandBuffer, DescriptorSetIndex index, const DescriptorSet &set);
    void pushConstants(VkCommandBuffer commandBuffer, const void *data, size_t size);
private:
    std::vector<VkDescriptorSetLayoutBinding> createGlobalUniformDataLayoutBindings();
    void createPipelineLayout(const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);
    void createPipeline(const Shader &vertexShader, const Shader &fragmentShader);

    Device &device;
    const RenderPass &renderPass;
    const Material &material;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    DescriptorSetLayout &materialDescriptorSetLayout;
};

#endif
