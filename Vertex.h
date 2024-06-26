#pragma once
#include "MathHelpers.h"
#include <array>
#include<vulkan/vulkan.h>


static VkFormat posFormat = VK_FORMAT_R32G32B32_SFLOAT;
static VkFormat normalFormat = VK_FORMAT_R32G32B32_SFLOAT;
static VkFormat tangentFormat = VK_FORMAT_R32G32B32_SFLOAT;
static VkFormat texcoordFormat = VK_FORMAT_R32G32_SFLOAT;
static VkFormat colFormat = VK_FORMAT_R32G32B32_SFLOAT;


//TODO: Split into simple (pos,normal,color,node) and standard (pos, normal,tangent,texcoord,color,node) in the future
struct Vertex {
	float_3 pos;
	float_3 normal;
	float_4 tangent;
	float_2 texcoord;
	float_3 color;
	int node;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, texcoord);
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, color);
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32_SINT;
		attributeDescriptions[5].offset = offsetof(Vertex, node);
		return attributeDescriptions;
	};
};