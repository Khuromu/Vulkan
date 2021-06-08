#pragma once

struct Vertex
{
	glm::vec3 pos;
	//glm::vec3 color;
	glm::vec3 norm;
	glm::vec2 uv;

	glm::vec3 tangent;
	glm::vec3 bitangent;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; //VK_FORMAT_R32G32_SFLOAT for Vec2
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, norm);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, uv);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const 
	{
		return pos == other.pos && norm == other.norm && uv == other.uv;
	}
};

struct Controller
{
	float speed = 1.f;

	float horizontal = 0.f;
	float vertical = 0.f;
	float perpendicular = 0.f;

	float altHor = 0.f;
	float altVer = 0.f;

	float yDir = 0.f;

	float lightIntensity = 0.f;
};


struct VulkanRenderContext
{
	static const int PENDING_FRAMES = 2;

	VulkanDeviceContext* context;

	uint32_t graphicsQueueIndex;
	uint32_t presentQueueIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkCommandPool mainCommandPool;
	VkCommandBuffer mainCommandBuffers[PENDING_FRAMES];
	VkFence mainFences[PENDING_FRAMES];
	VkFramebuffer framebuffers[PENDING_FRAMES];
	VkRenderPass renderPass;

	VkImageSubresourceRange mainSubRange;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;	//Used to draw a square
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkBuffer stagingBuffer; //TO DO Dynamic Array
	VkDeviceMemory stagingBufferMemory;

	VkImage albedoImage;
	VkDeviceMemory albedoImageMemory;
	VkImageView albedoImageView;
	VkSampler albedoSampler;

	VkImage normalImage;
	VkDeviceMemory normalImageMemory;
	VkImageView normalImageView;
	VkSampler normalSampler;

	VkImage metallicImage;
	VkDeviceMemory metallicImageMemory;
	VkImageView metallicImageView;
	VkSampler metallicSampler;

	VkImage	roughnessImage;
	VkDeviceMemory roughnessImageMemory;
	VkImageView	roughnessImageView;
	VkSampler roughnessSampler;

	VkImage	aoImage;
	VkDeviceMemory aoImageMemory;
	VkImageView	aoImageView;
	VkSampler aoSampler;

	VkImage skyboxImage;
	VkDeviceMemory skyboxImageMemory;
	VkImageView skyboxImageView;
	VkSampler skyboxSampler;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
};