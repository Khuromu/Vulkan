#pragma once

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
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkImageSubresourceRange mainSubRange;
};
