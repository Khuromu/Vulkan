#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "DataStructs.h"

struct MeshData
{
	glm::vec3 albedo = glm::vec3(1.0f, 0.0f, 0.0f);
	float metallic = 0.f;
	float roughness = 0.1f;
	float ao = 0.05f;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		//Position
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		//UV
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, uv);

		//Normals
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<int> indices;

	MeshData mData;

	void GenerateSphere(int horizSegments, int vertiSegments, float sphereScale = 1.f)
	{
		const float PI = 3.14159265359f;

		indices.reserve((vertiSegments + 1) * (horizSegments + 1)*2);
		vertices.reserve((vertiSegments + 1) * (horizSegments + 1));

		for (unsigned int y = 0; y <= vertiSegments; ++y)
		{
			for (unsigned int x = 0; x <= horizSegments; ++x)
			{
				float xSegment = (float)x / (float)horizSegments;
				float ySegment = (float)y / (float)vertiSegments;
				float theta = ySegment * PI;
				float phi = xSegment * 2.0f * PI;
				float xPos = std::cos(phi) * std::sin(theta);
				float yPos = std::cos(theta);
				float zPos = std::sin(phi) * std::sin(theta);
				Vertex vtx
				{
					glm::vec3(xPos * sphereScale, yPos * sphereScale, zPos * sphereScale),
					glm::vec2(xSegment, ySegment),
					glm::vec3(xPos, yPos, zPos)
				};
				vertices.push_back(vtx);
			}
		}

		bool oddRow = false;
		for (int y = 0; y < vertiSegments; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (int x = 0; x <= horizSegments; ++x)
				{
					// (y) suivi de (y+1) -> CW
					// (y+1) suivi de (y) -> CCW
					indices.push_back((y + 1) * (horizSegments + 1) + x);
					indices.push_back(y * (horizSegments + 1) + x);
				}
			}
			else
			{
				for (int x = horizSegments; x >= 0; --x)
				{
					// (y+1) suivi de (y) -> CW
					// (y) suivi de (y+1) -> CCW
					indices.push_back(y * (horizSegments + 1) + x);
					indices.push_back((y + 1) * (horizSegments + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
	}

};

struct UniformBufferObject 
{
	//Changing with each mesh
    alignas(16) glm::mat4 model;
	//Changing with camera position
    alignas(16) glm::mat4 view;
	//Constant
    alignas(16) glm::mat4 proj;
	//Constant
	alignas(16) glm::vec3 lightPos;
	//Might change
	alignas(16) glm::vec3 lightCol;
};