#pragma once

#include "vk_common.h"
#include "DeviceContext.h"
#include "RenderContext.h"

struct GLFWwindow;

struct VulkanGraphicsApplication
{
	const char* name;
	VulkanDeviceContext context;
	VulkanRenderContext rendercontext;
	GLFWwindow* window;

	//

	uint32_t m_imageIndex;
	uint32_t m_frame;
	uint32_t m_currentFrame;

	bool Initialize(const char *);
	bool Prepare();
	bool Run();
	bool Update();
	bool Begin();
	bool End();
	bool Display();
	void Terminate();
	bool Shutdown();

	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void CreateTextureImage(const std::string path, VkImage& image, VkDeviceMemory& deviceMemory, VkFormat imageFormat);
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateUniformBuffer(uint32_t currentImage);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void CreateFrameBuffers();

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateTextureImageViews();
	void CreateTextureSampler();

	void CreateDepthResources();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat FindDepthFormat();

	void LoadModel();
	void ComputeTangentBasis(std::vector<Vertex>& vertices);
};
