#ifndef RESOURCE_H_
#define RESOURCE_H_

#include <cstddef>
#include <cstdint>
#include <glm/vec3.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class Mesh;

typedef uint64_t ResourceId;

template<typename T>
class Resource
{
    friend class ResourceRepository;

public:
    ResourceId getId() const;
    T &getData();
    const T& getData() const;

private:
    Resource(ResourceId id, std::unique_ptr<T> data);
public:
    Resource(const Resource<T> &) = delete;
    Resource(Resource<T> &&) = default;
    ~Resource() = default;

private:
    ResourceId id;
    std::unique_ptr<T> data;
};

template<typename T>
Resource<T>::Resource(ResourceId id, std::unique_ptr<T> data)
    : id(id),
    data(std::move(data))
{
    if (!this->data) {
        throw std::invalid_argument("'data' must not be a null pointer");
    }
}

template<typename T>
ResourceId Resource<T>::getId() const
{
    return id;
}

template<typename T>
T &Resource<T>::getData()
{
    return *data.get();
}

template<typename T>
const T &Resource<T>::getData() const
{
    return *data.get();
}

struct ShaderResourceData
{
    VkShaderStageFlags stage;
    std::vector<std::byte> code;
    std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings;
};
typedef Resource<ShaderResourceData> ShaderResource;

struct ImageResourceData
{
    uint32_t width;
    uint32_t height;
    void *data;
};
typedef Resource<ImageResourceData> ImageResource;

struct MaterialResourceData
{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    
    const ImageResource *ambientTexture;
    const ImageResource *diffuseTexture;
    const ImageResource *specularTexture;
    const ImageResource *normalTexture;

    const ShaderResource *vertexShader;
    const ShaderResource *fragmentShader;

    std::string name;
};
typedef Resource<MaterialResourceData> MaterialResource;

typedef Resource<Mesh> MeshResource;

#endif
