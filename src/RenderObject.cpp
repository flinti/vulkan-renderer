#include "RenderObject.h"
#include "Device.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <utility>
#include <vulkan/vulkan.h>
#include "../third-party/spdlog/include/spdlog/spdlog.h"

RenderObject::RenderObject(
    uint32_t id,
    Device &device,
    const MeshResource &mesh,
    const Material &material,
    std::string name
)
    : device(&device),
    material(&material),
    transform(1.f), 
    indexCount(mesh.getData().getIndexCount()),
    vertexCount(mesh.getData().getVertexCount()),
    indexType(mesh.getData().getIndexType()),
    vertexBuffer(
        device.getAllocator(), 
        (void *) mesh.getData().getVertexData().data(), 
        mesh.getData().getVertexDataSize(), 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    ),
    indexBuffer(
        indexCount > 0 
        ? std::make_unique<Buffer>(
            device.getAllocator(), 
            (void *) mesh.getData().getIndexData().data(), 
            mesh.getData().getIndexDataSize(), 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        : nullptr
    ),
    id(id),
    name(name)
{
}

RenderObject::RenderObject(RenderObject &&other)
    : device(other.device),
    material(other.material),
    transform(other.transform),
    indexCount(other.indexCount),
    vertexCount(other.vertexCount),
    indexType(other.indexType),
    vertexBuffer(std::move(other.vertexBuffer)),
    indexBuffer(std::move(other.indexBuffer)),
    id(other.id),
    name(std::move(other.name))
{
    other.device = nullptr;
    other.material = nullptr;
}

RenderObject::~RenderObject()
{
    indexBuffer.reset();
}


RenderObject &RenderObject::operator =(RenderObject &&other)
{
    device = other.device;
    material = other.material;
    transform = other.transform;
    indexCount = other.indexCount;
    vertexCount = other.vertexCount;
    indexType = other.indexType;
    vertexBuffer = std::move(other.vertexBuffer);
    indexBuffer = std::move(other.indexBuffer);
    id = other.id;
    name = std::move(other.name);

    other.device = nullptr;
    other.material = nullptr;

    return *this;
}


std::vector<VkDescriptorSetLayoutBinding> RenderObject::getGlobalUniformDataLayoutBindings()
{
	return std::vector<VkDescriptorSetLayoutBinding>{
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		},
	};
}

uint32_t RenderObject::getId() const
{
    return id;
}

const glm::mat4 &RenderObject::getTransform() const
{
    return transform;
}

void RenderObject::setTransform(const glm::mat4 &transform)
{
    this->transform = transform;
}

const Material &RenderObject::getMaterial() const
{
    return *material;
}

void RenderObject::enqueueDrawCommands(VkCommandBuffer commandBuffer) const
{
    VkBuffer vertexBuffers[] = {vertexBuffer.getHandle()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    if (indexCount > 0) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getHandle(), 0, indexType);

        vkCmdDrawIndexed(
            commandBuffer, 
            indexCount, 
            1, 
            0, 
            0, 
            0
        );
    }
    else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}
