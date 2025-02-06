#ifndef RENDEROBJECT_H_
#define RENDEROBJECT_H_

#include "Buffer.h"
#include "Image.h"
#include "Resource.h"

#include <map>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>

class Material;
class Device;

struct GlobalUniformData
{
    glm::mat4 viewProj;
    glm::vec3 viewPos;
    float pad1;
    glm::vec4 time;
    glm::vec3 lightPosition;
    float pad2;
    glm::vec3 lightColor;
    float pad3;
};

class RenderObject
{
public:
    RenderObject(
        uint32_t id,
        Device &device,
        const MeshResource &mesh,
        const Material &material,
        std::string name = ""
    );
    RenderObject(const RenderObject &) = delete;
    RenderObject(RenderObject &&);
    ~RenderObject();

    RenderObject &operator =(RenderObject &&);

    static std::vector<VkDescriptorSetLayoutBinding> getGlobalUniformDataLayoutBindings();

    uint32_t getId() const;
    const glm::mat4 &getTransform() const;
    void setTransform(const glm::mat4 &transform);
    const Material &getMaterial() const;

    void enqueueDrawCommands(VkCommandBuffer commandBuffer) const;
private:
    Device *device;
    const Material *material;
    glm::mat4 transform;
    uint32_t indexCount;
    uint32_t vertexCount;
    VkIndexType indexType;
    Buffer vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    uint32_t id;
    std::string name;
};

#endif
