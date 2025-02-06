#include "Mesh.h"
#include "Utility.h"
#include <glm/detail/qualifier.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>



Mesh::Mesh(std::vector<Vertex> vertices, std::vector<IndexType> indices, const MaterialResource *material)
	: material(material),
	vertices(std::move(vertices)),
	indices(std::move(indices))
{
}


const MaterialResource *Mesh::getMaterial() const
{
	return material;
}

const std::vector<Vertex> &Mesh::getVertexData() const
{
    return vertices;
}

size_t Mesh::getVertexDataSize() const
{
	return vertices.size() * sizeof(vertices[0]);
}

uint32_t Mesh::getVertexCount() const
{
	return vertices.size();
}

const std::vector<Mesh::IndexType> &Mesh::getIndexData() const
{
    return indices;
}

size_t Mesh::getIndexDataSize() const
{
	return indices.size() * sizeof(indices[0]);
}

uint32_t Mesh::getIndexCount() const
{
	return indices.size();
}


VkIndexType Mesh::getIndexType() const
{
	return VK_INDEX_TYPE_UINT32;
}

Mesh Mesh::createRegularPolygon(float r, uint32_t edges, glm::vec3 offset)
{
    Mesh mesh;
	mesh.vertices.reserve(edges + 1);
	mesh.indices.reserve(edges * 3);

	for (uint32_t i = 0; i < edges; ++i) {
		float phi = 2 * 3.1415926f / edges * i;
		float x = r * glm::cos(phi);
		float z = r * glm::sin(phi);

		Vertex v{
			.position = { x + offset.x, offset.y, z + offset.z },
			.normal = { 0.f, 1.f, 0.f },
			.color = Utility::colorFromHsl(360.f / edges * i, 1.f, 0.5f),
			.uv = { 0.f, 0.f },
		};
		mesh.vertices.push_back(v);
		mesh.indices.push_back(edges);
		mesh.indices.push_back(i);
		mesh.indices.push_back((i + 1) % edges);
	}
	mesh.vertices.emplace_back(Vertex{
		.position = offset, 
		.normal = { 0.f, 1.f, 0.f },
		.color = { 0.f, 0.f, 0.f },
		.uv = { 0.f, 0.f },
	});

    return mesh;
}

Mesh Mesh::createPlane(glm::vec3 a, glm::vec3 b, glm::vec3 offset)
{
	Mesh mesh;
	glm::vec3 normal = glm::normalize(glm::cross(a, b));
	mesh.vertices = {
		{
			.position = offset,
			.normal = normal,
			.color = { 1.f, 0.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a, 
			.normal = normal,
			.color = { 0.f, 1.f, 0.f },
			.uv = { 1.f, 0.f },
		},
		{
			.position = offset + a + b, 
			.normal = normal,
			.color = { 0.f, 0.f, 1.f },
			.uv = { 1.f, 1.f },
		},
		{
			.position = offset + b, 
			.normal = normal,
			.color = { 1.f, 1.f, 1.f },
			.uv = { 0.f, 1.f },
		},
	};
	mesh.indices = { 0, 1, 2, 2, 3, 0};

	return mesh;
}

Mesh Mesh::createTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 offset)
{
	Mesh mesh;
	glm::vec3 normal = glm::normalize(glm::cross(a, b));
	mesh.vertices = {
		{
			.position = offset,
			.normal = normal,
			.color = { 1.f, 0.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a,
			.normal = normal,
			.color = { 0.f, 1.f, 0.f },
			.uv = { 1.f, 0.f },
		},
		{
			.position = offset + a + b,
			.normal = normal,
			.color = { 0.f, 0.f, 1.f },
			.uv = { 1.f, 1.f },
		},
	};
	mesh.indices = { 0, 1, 2 };

	return mesh;
}

Mesh Mesh::createUnitCube()
{
	Mesh mesh;
	mesh.vertices = {
        { .position = { -0.5f, -0.5f, -0.5f },  .normal = {0.f, 0.f, -1.f}, .uv = { 0.0f, 0.0f } },
        { .position = { 0.5f, -0.5f, -0.5f }, .normal = {0.f, 0.f, -1.f}, .uv = { 1.0f, 0.0f } },
        { .position = { 0.5f,  0.5f, -0.5f }, .normal = {0.f, 0.f, -1.f}, .uv = { 1.0f, 1.0f } },
        { .position = { -0.5f,  0.5f, -0.5f }, .normal = {0.f, 0.f, -1.f}, .uv = { 0.0f, 1.0f } },

		{ .position = {  0.5f,  0.5f,  0.5f }, .normal = {0.f, 0.f, 1.f}, .uv = { 1.0f, 1.0f } },
        { .position = { 0.5f, -0.5f,  0.5f }, .normal = {0.f, 0.f, 1.f}, .uv = { 1.0f, 0.0f } },
        { .position = { -0.5f, -0.5f,  0.5f }, .normal = {0.f, 0.f, 1.f}, .uv = { 0.0f, 0.0f } },
        { .position = { -0.5f,  0.5f,  0.5f }, .normal = {0.f, 0.f, 1.f}, .uv = { 0.0f, 1.0f } },

        { .position = { -0.5f, -0.5f, -0.5f }, .normal = {-1.f, 0.f, 0.f}, .uv = { 0.0f, 1.0f } },
        { .position = { -0.5f,  0.5f, -0.5f }, .normal = {-1.f, 0.f, 0.f}, .uv = { 1.0f, 1.0f } },
		{ .position = { -0.5f,  0.5f,  0.5f }, .normal = {-1.f, 0.f, 0.f}, .uv = { 1.0f, 0.0f } },
        { .position = { -0.5f, -0.5f,  0.5f }, .normal = {-1.f, 0.f, 0.f}, .uv = { 0.0f, 0.0f } },

        { .position = { 0.5f,  0.5f,  0.5f }, .normal = {1.f, 0.f, 0.f}, .uv = { 1.0f, 0.0f } },
        { .position = { 0.5f,  0.5f, -0.5f }, .normal = {1.f, 0.f, 0.f}, .uv = { 1.0f, 1.0f } },
        { .position = { 0.5f, -0.5f, -0.5f }, .normal = {1.f, 0.f, 0.f}, .uv = { 0.0f, 1.0f } },
        { .position = { 0.5f, -0.5f,  0.5f }, .normal = {1.f, 0.f, 0.f}, .uv = { 0.0f, 0.0f } },

        { .position = { 0.5f, -0.5f,  0.5f }, .normal = {0.f, -1.f, 0.f}, .uv = { 1.0f, 0.0f } },
        { .position = { 0.5f, -0.5f, -0.5f }, .normal = {0.f, -1.f, 0.f}, .uv = { 1.0f, 1.0f } },
        { .position = { -0.5f, -0.5f, -0.5f }, .normal = {0.f, -1.f, 0.f}, .uv = { 0.0f, 1.0f } },
        { .position = { -0.5f, -0.5f,  0.5f }, .normal = {0.f, -1.f, 0.f}, .uv = { 0.0f, 0.0f } },

        { .position = { -0.5f,  0.5f, -0.5f }, .normal = {0.f, 1.f, 0.f}, .uv = { 0.0f, 1.0f } },
        { .position = { 0.5f,  0.5f, -0.5f }, .normal = {0.f, 1.f, 0.f}, .uv = { 1.0f, 1.0f } },
        { .position = { 0.5f,  0.5f,  0.5f }, .normal = {0.f, 1.f, 0.f}, .uv = { 1.0f, 0.0f } },
        { .position = { -0.5f,  0.5f,  0.5f }, .normal = {0.f, 1.f, 0.f}, .uv = { 0.0f, 0.0f } },
    };
	mesh.indices = {
		0, 1, 2, 2, 3, 0, 
		4, 5, 6, 6, 7, 4, 
		8, 9, 10, 10, 11, 8, 
		12, 13, 14, 14, 15, 12, 
		16, 17, 18, 18, 19, 16, 
		20, 21, 22, 22, 23, 20,
	};

	return mesh;
}
