
#include "vk_common.h"
#include <chrono>

#include "DeviceContext.h"
#include "RenderContext.h"
#include "GraphicsApplication.h"
#include "ShaderLink.h"

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define APP_NAME "00_minimal"

Mesh mesh;
Transform camTrans;
Transform lightTrans;
Controller ctrl;

//User
static std::vector<char> ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			ctrl.speed = 10.f;
		}

		if (key == GLFW_KEY_W)
		{
			//Forward
			ctrl.perpendicular = -1.f;
		}
		else if (key == GLFW_KEY_S)
		{
			//Backward
			ctrl.perpendicular = 1.f;
		}

		if (key == GLFW_KEY_A)
		{
			//Left
			ctrl.horizontal = -1.f;
		}
		else if (key == GLFW_KEY_D)
		{
			//Right
			ctrl.horizontal = 1.f;
		}

		if (key == GLFW_KEY_PAGE_UP)
		{
			//Up
			ctrl.vertical = 1.f;
		}
		else if (key == GLFW_KEY_PAGE_DOWN)
		{
			//Down
			ctrl.vertical = -1.f;
		}

		if (key == GLFW_KEY_KP_4)
		{
			//Left Light
			ctrl.altHor = -1.f;
		}
		else if (key == GLFW_KEY_KP_6)
		{
			//Right Light
			ctrl.altHor = 1.f;
		}

		if (key == GLFW_KEY_KP_5)
		{
			//Left Light
			ctrl.altVer = -1.f;
		}
		else if (key == GLFW_KEY_KP_8)
		{
			//Right Light
			ctrl.altVer = 1.f;
		}

		//if (key == GLFW_KEY_Q)
		//{
		//	//Rotate left
		//	ctrl.yDir = -1.f;
		//}
		//else if (key == GLFW_KEY_E)
		//{
		//	//Rotate right
		//	ctrl.yDir = +1.f;
		//}
	}
	else if (action == GLFW_RELEASE)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			ctrl.speed = 1.f;
		}

		if (key == GLFW_KEY_W || key == GLFW_KEY_S)
		{
			//Perpendicular
			ctrl.perpendicular = 0.f;
		}

		if (key == GLFW_KEY_A || key == GLFW_KEY_D)
		{
			//Horizontal
			ctrl.horizontal = 0.f;
		}

		if (key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_PAGE_DOWN)
		{
			//Vertical
			ctrl.vertical = 0.f;
		}

		if (key == GLFW_KEY_Q || key == GLFW_KEY_E)
		{
			//Rotate
			ctrl.yDir = 0.f;
		}

		if (key == GLFW_KEY_KP_4 || key == GLFW_KEY_KP_6)
		{
			//Light Horizontal
			ctrl.altHor = 0.f;
		}

		if (key == GLFW_KEY_KP_5 || key == GLFW_KEY_KP_8)
		{
			//Light Vertical
			ctrl.altVer = 0.f;
		}
	}
}

VkShaderModule VulkanGraphicsApplication::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");

	return shaderModule;
}

void VulkanGraphicsApplication::CreateShaders()
{
	std::vector<char> vertShaderCompiled = ReadFile("../../Shaders/PBVert.spv");
	std::vector<char> fragShaderCompiled = ReadFile("../../Shaders/PBFrag.spv");

	rendercontext.vertShaderModule = CreateShaderModule(vertShaderCompiled);
	rendercontext.fragShaderModule = CreateShaderModule(fragShaderCompiled);
}

void VulkanGraphicsApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("echec de la creation d'un vertex buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = context.findMemoryType(memRequirements.memoryTypeBits,
		properties);

	if (vkAllocateMemory(context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("echec d'une allocation de memoire!");

	vkBindBufferMemory(context.device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanGraphicsApplication::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = rendercontext.mainCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanGraphicsApplication::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(rendercontext.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(rendercontext.graphicsQueue);

	vkFreeCommandBuffers(context.device, rendercontext.mainCommandPool, 1, &commandBuffer);
}

void VulkanGraphicsApplication::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	//Should make this commandBuffer available in a class or something
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		throw std::invalid_argument("unsupported layout transition!");

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);

}

void VulkanGraphicsApplication::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	//Should make this commandBuffer available in a class or something
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = 
	{
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage
	(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
	
	EndSingleTimeCommands(commandBuffer);
}

void VulkanGraphicsApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	//Should make this commandBuffer available in a class or something
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanGraphicsApplication::CreateImage(uint32_t width, uint32_t height, VkFormat format, 
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(context.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = context.findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(context.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(context.device, image, imageMemory, 0);
}

void VulkanGraphicsApplication::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		rendercontext.stagingBuffer, rendercontext.stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, rendercontext.stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(context.device, rendercontext.stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, 
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		rendercontext.textureImage, rendercontext.textureImageMemory);

	TransitionImageLayout(rendercontext.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(rendercontext.stagingBuffer, rendercontext.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	TransitionImageLayout(rendercontext.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	TransitionImageLayout(rendercontext.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(context.device, rendercontext.stagingBuffer, nullptr);
	vkFreeMemory(context.device, rendercontext.stagingBufferMemory, nullptr);
}

VkImageView VulkanGraphicsApplication::CreateImageView(VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = rendercontext.textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(context.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");

	return imageView;
}

///Might be unecessary
//void VulkanGraphicsApplication::CreateImageViews()
//{
//	context.swapchainImageViews.resize(context.SWAPCHAIN_IMAGES);
//
//	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
//		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
//	}
//}

void VulkanGraphicsApplication::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	//!!Didn't do that for the device!!
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.textureSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

void VulkanGraphicsApplication::CreateTextureImageView()
{
	rendercontext.textureImageView = CreateImageView(rendercontext.textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}

void VulkanGraphicsApplication::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.device, stagingBufferMemory);
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, rendercontext.vertexBuffer, rendercontext.vertexBufferMemory);

	CopyBuffer(stagingBuffer, rendercontext.vertexBuffer, bufferSize);

	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void VulkanGraphicsApplication::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, mesh.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.device, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rendercontext.indexBuffer, rendercontext.indexBufferMemory);

	CopyBuffer(stagingBuffer, rendercontext.indexBuffer, bufferSize);

	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void VulkanGraphicsApplication::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	rendercontext.uniformBuffers.resize(context.SWAPCHAIN_IMAGES);
	rendercontext.uniformBuffersMemory.resize(context.SWAPCHAIN_IMAGES);

	for (size_t i = 0; i < context.SWAPCHAIN_IMAGES; i++)
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			rendercontext.uniformBuffers[i], rendercontext.uniformBuffersMemory[i]);
}

void VulkanGraphicsApplication::CreateDescriptorPool()
{
	///Am I breaking anything ?
	/*VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);*/

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);

	if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr, &rendercontext.descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");
}

void VulkanGraphicsApplication::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(context.SWAPCHAIN_IMAGES, rendercontext.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = rendercontext.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	allocInfo.pSetLayouts = layouts.data();

	rendercontext.descriptorSets.resize(context.SWAPCHAIN_IMAGES);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, rendercontext.descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets!");

	for (size_t i = 0; i < context.SWAPCHAIN_IMAGES; i++) 
	{
		///Textures change everything
		/*VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = rendercontext.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);*/
		
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = rendercontext.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = rendercontext.textureImageView;
		imageInfo.sampler = rendercontext.textureSampler;

		//VkWriteDescriptorSet descriptorWrite{};
		//descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//descriptorWrite.dstSet = rendercontext.descriptorSets[i];
		//descriptorWrite.dstBinding = 0;
		//descriptorWrite.dstArrayElement = 0;
		//descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		//descriptorWrite.descriptorCount = 1;
		//descriptorWrite.pBufferInfo = &bufferInfo;
		//descriptorWrite.pImageInfo = nullptr; // Optional
		//descriptorWrite.pTexelBufferView = nullptr; // Optional

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		//vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void VulkanGraphicsApplication::UpdateUniformBuffer(uint32_t currentImage)
{
	static std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.f), 0.f, glm::vec3(0.f, 1.f, 0.f));
	//ubo.model = glm::translate(glm::mat4(1.0f), modelTrans.position);;
	//Coord spherique vers cartesienne
	ubo.view = glm::lookAt(camTrans.position, camTrans.position + glm::vec3(0.f,0.f, -1.f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), context.swapchainExtent.width / (float)context.swapchainExtent.height, 0.1f, 100.0f);
	ubo.proj[1][1] *= -1;
	
	ubo.lightPos = lightTrans.position;
	ubo.lightCol = glm::vec3(23.47, 21.31, 20.79);

	void* data;
	vkMapMemory(context.device, rendercontext.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(context.device, rendercontext.uniformBuffersMemory[currentImage]);
}


//Framework

bool VulkanGraphicsApplication::Prepare()
{
	//Create sphere to use index and vertices
	mesh.GenerateSphere(100, 100);
	m_imageIndex = 0;

	// creer les semaphores
	VkSemaphoreCreateInfo semCreateInfo = {};
	semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	// il faut 2 semaphores par image : 1 pour l'acquire et 1 pour le present 
	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++) 
	{
		DEBUG_CHECK_VK(vkCreateSemaphore(context.device, &semCreateInfo, nullptr, &context.acquireSemaphores[i]));
		DEBUG_CHECK_VK(vkCreateSemaphore(context.device, &semCreateInfo, nullptr, &context.presentSemaphores[i]));
	}
	CreateShaders();

	// creer le command pool
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	DEBUG_CHECK_VK(vkCreateCommandPool(context.device, &cmdPoolCreateInfo, nullptr, &rendercontext.mainCommandPool));

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkCommandBufferAllocateInfo cmdAllocInfo = {};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.commandPool = rendercontext.mainCommandPool;
	cmdAllocInfo.commandBufferCount = 1;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	for (int i = 0; i < rendercontext.PENDING_FRAMES; i++)
	{
		// creer les fences (en ETAT SIGNALE)
		DEBUG_CHECK_VK(vkCreateFence(context.device, &fenceCreateInfo, nullptr, &rendercontext.mainFences[i]));
		// creer les command buffers (1 par frame)
		DEBUG_CHECK_VK(vkAllocateCommandBuffers(context.device, &cmdAllocInfo, &rendercontext.mainCommandBuffers[i]));
	}

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optionnel

	///This might be not good!
	/*VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;*/

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &rendercontext.descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("echec de la creation d'un set de descripteurs!");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = rendercontext.vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = rendercontext.fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::getAttributeDescriptions().size());
	vertexInputInfo.pVertexBindingDescriptions = &Vertex::getBindingDescription();
	vertexInputInfo.pVertexAttributeDescriptions = Vertex::getAttributeDescriptions().data();
	
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)context.swapchainExtent.width;
	viewport.height = (float)context.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkDynamicState dynamicStates[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;
	
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = context.surfaceFormat.format;;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;


	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &rendercontext.renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");

	//VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	//pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//pipelineLayoutInfo.setLayoutCount = 0; // Optional
	//pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	//pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	//pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	//Constant push, unstable
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0; //optional ?
	pushConstantRange.size = sizeof(MeshData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &rendercontext.descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &rendercontext.pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = /*&dynamicState*/nullptr; // Optional
	pipelineInfo.layout = rendercontext.pipelineLayout;
	pipelineInfo.renderPass = rendercontext.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = rendercontext.renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = context.swapchainExtent.width;
	framebufferInfo.height = context.swapchainExtent.height;
	framebufferInfo.layers = 1;

	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = context.surfaceFormat.format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++)
	{
		imageViewCreateInfo.image = context.swapchainImages[i];
		framebufferInfo.pAttachments = &context.swapchainImageViews[i];
		if (vkCreateImageView(context.device, &imageViewCreateInfo, nullptr, &context.swapchainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image views");
		if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &rendercontext.framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}
	//Crash, probably something to do with the pipeline info
	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rendercontext.graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

	return true;
}

void VulkanGraphicsApplication::Terminate()
{
	// tout detruire ici
	vkDestroyRenderPass(context.device, rendercontext.renderPass, nullptr);
	vkFreeMemory(context.device, rendercontext.indexBufferMemory, nullptr);
	vkDestroyBuffer(context.device, rendercontext.vertexBuffer, nullptr);
	vkFreeMemory(context.device, rendercontext.vertexBufferMemory, nullptr);
	vkDestroyPipelineLayout(context.device, rendercontext.pipelineLayout, nullptr);
	vkDestroyPipeline(context.device, rendercontext.graphicsPipeline, nullptr);
	vkDestroyShaderModule(context.device, rendercontext.fragShaderModule, nullptr);
	vkDestroyShaderModule(context.device, rendercontext.vertShaderModule, nullptr);
	
	for (int i = 0; i < context.SWAPCHAIN_IMAGES;i++)
	{
		vkDestroyImageView(context.device, context.swapchainImageViews[i], nullptr);
		vkDestroyFramebuffer(context.device, rendercontext.framebuffers[i], nullptr);
		vkDestroyBuffer(context.device, rendercontext.uniformBuffers[i], nullptr);
		vkFreeMemory(context.device, rendercontext.uniformBuffersMemory[i], nullptr);
		vkDestroySemaphore(context.device, context.acquireSemaphores[i], nullptr);
		vkDestroySemaphore(context.device, context.presentSemaphores[i], nullptr);
	}
	vkDestroySampler(context.device, rendercontext.textureSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.textureImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.textureImage, nullptr);
	vkFreeMemory(context.device, rendercontext.textureImageMemory, nullptr);
	vkDestroyDescriptorPool(context.device, rendercontext.descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(context.device, rendercontext.descriptorSetLayout, nullptr);
	vkDestroyBuffer(context.device, rendercontext.indexBuffer, nullptr);
	
	for (int i = 0; i < rendercontext.PENDING_FRAMES; i++)
		vkDestroyFence(context.device, rendercontext.mainFences[i], nullptr);
	
	vkDestroyCommandPool(context.device, rendercontext.mainCommandPool, nullptr);
}

//
// Frame
//

bool VulkanGraphicsApplication::Begin()
{
	//TODO: a retirer
	//vkDeviceWaitIdle(context.device);


	uint64_t timeout = UINT64_MAX;
	VkResult res;
	res = vkWaitForFences(context.device, 1, &rendercontext.mainFences[m_currentFrame], VK_TRUE, timeout);
	vkResetFences(context.device, 1, &rendercontext.mainFences[m_currentFrame]);
	vkResetCommandBuffer(rendercontext.mainCommandBuffers[m_currentFrame], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	DEBUG_CHECK_VK(vkAcquireNextImageKHR(context.device, context.swapchain, timeout, context.acquireSemaphores[m_imageIndex], VK_NULL_HANDLE, &m_imageIndex));

	//mesh.mData.trans.position.x = 3.f;
	glfwSetKeyCallback(window, KeyCallback);

	return true;
}

bool VulkanGraphicsApplication::End()
{
	VkSubmitInfo submitInfo = {};
	uint32_t stageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = stageMask;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &rendercontext.mainCommandBuffers[m_currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &context.presentSemaphores[m_imageIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &context.acquireSemaphores[m_imageIndex];

	vkQueueSubmit(rendercontext.graphicsQueue, 1, &submitInfo, rendercontext.mainFences[m_currentFrame]);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &context.presentSemaphores[m_imageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &context.swapchain;
	presentInfo.pImageIndices = &m_imageIndex;
	DEBUG_CHECK_VK(vkQueuePresentKHR(rendercontext.presentQueue, &presentInfo));

	m_frame++;
	m_currentFrame = m_frame % rendercontext.PENDING_FRAMES;

	return true;
}

bool VulkanGraphicsApplication::Update()
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	static double previousTime = glfwGetTime();
	static double currentTime = glfwGetTime();

	currentTime = glfwGetTime();
	double deltaTime = currentTime - previousTime;
	std::cout << "[" << m_frame << "] frame time = " << deltaTime*1000.0 << " ms [" << 1.0 / deltaTime << " fps]" << std::endl;
	previousTime = currentTime;

	float time = (float)currentTime;

	camTrans.position.x += ctrl.horizontal * deltaTime * ctrl.speed;
	camTrans.position.y += ctrl.vertical * deltaTime * ctrl.speed;
	camTrans.position.z += ctrl.perpendicular * deltaTime * ctrl.speed;

	lightTrans.position.x += ctrl.altHor * deltaTime * ctrl.speed;
	lightTrans.position.y += ctrl.altVer * deltaTime * ctrl.speed;

	return true;
}

bool VulkanGraphicsApplication::Display()
{
	VkCommandBuffer cmd = rendercontext.mainCommandBuffers[m_currentFrame];
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &cmdBeginInfo);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.clearValueCount = 1;
	VkClearValue clearvalues;
	clearvalues.color = { 0.01f,0.01f,0.01f,1.f };
	renderPassBeginInfo.framebuffer = rendercontext.framebuffers[m_imageIndex];
	renderPassBeginInfo.renderPass = rendercontext.renderPass;
	renderPassBeginInfo.pClearValues = &clearvalues;
	renderPassBeginInfo.renderArea.extent = context.swapchainExtent;
	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, rendercontext.graphicsPipeline);

	UpdateUniformBuffer(m_currentFrame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkBuffer vertexBuffers[] = { rendercontext.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

	//push constants here
	vkCmdPushConstants(cmd, rendercontext.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshData), &mesh.mData);

	vkCmdBindIndexBuffer(cmd, rendercontext.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, rendercontext.pipelineLayout, 0, 1, &rendercontext.descriptorSets[m_currentFrame], 0, nullptr);
	//vkCmdDraw(cmd, 3, 1, 0, 0);
	//vkcmddrawindex
	//PushConstant pour tout ce qui est dynamique
	//Tableau de matrices world
	//Ubo dynamiques
	//Update les descriptor sets dans le cmdBuffer (vkTemplate...)
	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
	vkCmdEndRenderPass(cmd);
	vkEndCommandBuffer(cmd);
	return true;
}

int main(void)
{
	/* Initialize the library */
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	VulkanGraphicsApplication app;	

	/* Create a windowed mode window and its OpenGL context */
	app.window = glfwCreateWindow(1024, 768, APP_NAME, NULL, NULL);
	if (!app.window)
	{
		glfwTerminate();
		return -1;
	}

	app.Initialize(APP_NAME);

	camTrans.position = glm::vec3(0.0f, 0.0f, 10.0f);
	lightTrans.position = glm::vec3(0.f, 0.f, 4.f);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(app.window))
	{
		/* Render here */
		app.Run();

		/* Poll for and process events */
		glfwPollEvents();
	}

	app.Shutdown();

	glfwTerminate();
	return 0;
}
