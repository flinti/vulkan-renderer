#ifndef RESOURCEREPOSITORY_H_
#define RESOURCEREPOSITORY_H_

#include "Material.h"
#include "Mesh.h"
#include "Image.h"
#include "Resource.h"
#include "Shader.h"
#include "../third-party/spirv-reflect/spirv_reflect.h"
#include "../third-party/tinyobjloader/tiny_obj_loader.h"
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
typedef std::string ResourceKey;

class ResourceRepository
{
public:
    ResourceRepository(const ResourceKey &defaultImage = "");
    ResourceRepository(const ResourceRepository &) = delete;
    ResourceRepository(ResourceRepository &&) = default;
    ~ResourceRepository();

    bool hasImage(const ResourceKey &name) const;

    const MeshResource &getMesh(const ResourceKey &name) const;
    const MaterialResource &getMaterial(const ResourceKey &name) const;
    const ImageResource &getImage(const ResourceKey &name) const;
    const ShaderResource &getFragmentShader(const ResourceKey &name) const;
    const ShaderResource &getVertexShader(const ResourceKey &name) const;
    
    void loadObj(const ResourceKey &name, const std::filesystem::path &path);
    void loadImage(const ResourceKey &name, const std::filesystem::path &path);
    void loadFragmentShader(const ResourceKey &name, const std::filesystem::path &path);
    void loadVertexShader(const ResourceKey &name, const std::filesystem::path &path);

    std::string resourceTree(size_t indentationLevel = 0) const;
private:
    void load(const std::filesystem::path &path, const std::string extension);
    void loadAll();

    Shader::DescriptorSetLayoutBindingMap getShaderBindings(const spv_reflect::ShaderModule &code);
    std::vector<std::byte> readShaderFile(const std::filesystem::path &path);
    const MaterialResource *loadObjMaterial(const tinyobj::material_t &material);

    ResourceId nextResourceId = 1;

    std::unordered_map<ResourceKey, MeshResource> meshes;
    std::unordered_map<ResourceKey, MaterialResource> materials;
    std::unordered_map<ResourceKey, ImageResource> images;
    std::unordered_map<ResourceKey, ShaderResource> vertexShaders;
    std::unordered_map<ResourceKey, ShaderResource> fragmentShaders;

    const MeshResource *defaultMesh = nullptr;
    const ImageResource *defaultImage = nullptr;
};

#endif
