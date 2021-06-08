#pragma once

#include "vk_common.h"
#include "DeviceContext.h"
#include "RenderContext.h"

struct GLFWwindow;

struct Controller
{
	float speed = 1.f;

	float horizontal = 0.f;
	float vertical = 0.f;
	float perpendicular = 0.f;

	float altHor = 0.f;
	float altVer = 0.f;

	float yDir = 0.f;
};

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

	//User
	void CreateShaders();
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
		VkImage& image, VkDeviceMemory& imageMemory);
	void CreateTextureImage();
	VkImageView CreateImageView(VkImage image, VkFormat format);
	//void CreateImageViews();
	void CreateTextureSampler();
	void CreateTextureImageView();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void UpdateUniformBuffer(uint32_t currentImage);

	//Framework
	bool Initialize(const char *);
	bool Prepare();
	bool Run();
	bool Update();
	bool Begin();
	bool End();
	bool Display();
	void Terminate();
	bool Shutdown();
};
