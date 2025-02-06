#ifndef VERTEX_H_
#define VERTEX_H_

#include "Utility.h"

#include <array>
#include <iostream>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    std::string to_string() const;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};

namespace std
{
    template <>
    struct hash<Vertex>
    {
        std::size_t operator()(const Vertex &v) const
        {
            using namespace Utility;

            std::size_t result = 0;

            hash_combine(result, v.position.x);
            hash_combine(result, v.position.y);
            hash_combine(result, v.position.z);
            hash_combine(result, v.normal.x);
            hash_combine(result, v.normal.y);
            hash_combine(result, v.normal.z);
            hash_combine(result, v.color.r);
            hash_combine(result, v.color.g);
            hash_combine(result, v.color.b);
            hash_combine(result, v.uv.x);
            hash_combine(result, v.uv.y);

            return result;
        }
    };
}

std::ostream &operator <<(std::ostream &s, const Vertex &v);

#endif
