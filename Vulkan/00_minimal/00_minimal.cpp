
#include "vk_common.h"

#include "DeviceContext.h"
#include "RenderContext.h"
#include "GraphicsApplication.h"

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES //To be sure model proj view mat are multple of 16 - can use alignas(16) too
#define GLM_FORCE_DEPTH_ZERO_TO_ONE //Vulkan projection matrix range of 0.0 to 1.0 instead of GLM of -1.0, 1.0
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader-master/tiny_obj_loader.h>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm/gtx/hash.hpp>

#include <chrono>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "../models/rapier.obj";
const std::string TEXTURE_PATH = "../textures/cube.png";

#define APP_NAME "00_minimal"

//
// Initialisation des ressources 
//
Controller ctrl;

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
			//forward
			ctrl.perpendicular = -1.f;
		}
		else if (key == GLFW_KEY_S)
		{
			//backward
			ctrl.perpendicular = 1.f;
		}

		if (key == GLFW_KEY_A)
		{
			//left
			ctrl.horizontal = -1.f;
		}
		else if (key == GLFW_KEY_D)
		{
			//right
			ctrl.horizontal = 1.f;
		}

		if (key == GLFW_KEY_UP)
		{
			//up
			ctrl.vertical = 1.f;
		}
		else if (key == GLFW_KEY_DOWN)
		{
			//down
			ctrl.vertical = -1.f;
		}

		if (key == GLFW_KEY_KP_4)
		{
			//left Light
			ctrl.altHor = -1.f;
		}
		else if (key == GLFW_KEY_KP_6)
		{
			//right Light
			ctrl.altHor = 1.f;
		}

		if (key == GLFW_KEY_KP_5)
		{
			//left Light
			ctrl.altVer = -1.f;
		}
		else if (key == GLFW_KEY_KP_8)
		{
			//right Light
			ctrl.altVer = 1.f;
		}

		/*if (key == GLFW_KEY_Q)
		{
		    //rotate left
		    ctrl.yDir = -1.f;
		}
		else if (key == GLFW_KEY_E)
		{
		    //rotate right
		    ctrl.yDir = +1.f;
		}*/

		if (key == GLFW_KEY_KP_0)
		{
			//light Intensity -
			ctrl.lightIntensity = -1.f;
		}

		else if (key == GLFW_KEY_KP_1)
		{
			//light Intensity +
			ctrl.lightIntensity = 1.f;
		}
	}

	else if (action == GLFW_RELEASE)
	{
		if (mods == GLFW_MOD_SHIFT)
		{
			ctrl.speed = 1.f;
		}

		if (key == GLFW_KEY_W || key == GLFW_KEY_S)
		{
			//perpendicular
			ctrl.perpendicular = 0.f;
		}

		if (key == GLFW_KEY_A || key == GLFW_KEY_D)
		{
			//horizontal
			ctrl.horizontal = 0.f;
		}

		if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
		{
			//vertical
			ctrl.vertical = 0.f;
		}

		if (key == GLFW_KEY_Q || key == GLFW_KEY_E)
		{
			//rotate
			ctrl.yDir = 0.f;
		}

		if (key == GLFW_KEY_KP_4 || key == GLFW_KEY_KP_6)
		{
			//light Horizontal
			ctrl.altHor = 0.f;
		}

		if (key == GLFW_KEY_KP_5 || key == GLFW_KEY_KP_8)
		{
			//light Vertical
			ctrl.altVer = 0.f;
		}

		if (key == GLFW_KEY_KP_0 || key == GLFW_KEY_KP_1)
		{
			//light Intensity
			ctrl.lightIntensity = 0.f;
		}
	}
}

/////////////// BUFFER ////////////////////
void VulkanGraphicsApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(context.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create vertex buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(context.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = context.FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(context.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate vertex buffer memory!");

	vkBindBufferMemory(context.device, buffer, bufferMemory, 0);
}


VkCommandBuffer VulkanGraphicsApplication::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = rendercontext.mainCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanGraphicsApplication::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(rendercontext.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(rendercontext.graphicsQueue);

	vkFreeCommandBuffers(context.device, rendercontext.mainCommandPool, 1, &commandBuffer);
}


void VulkanGraphicsApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	//copyRegion.srcOffset = 0; //optional
	//copyRegion.dstOffset = 0; //optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanGraphicsApplication::CreateFrameBuffers()
{
	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++)
	{
		std::array<VkImageView, 2> attachments = { context.swapchainImageViews[i], rendercontext.depthImageView };

		VkFramebufferCreateInfo frameBufferInfo = {};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = rendercontext.renderPass;
		frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferInfo.pAttachments = attachments.data();
		frameBufferInfo.width = context.swapchainExtent.width;
		frameBufferInfo.height = context.swapchainExtent.height;
		frameBufferInfo.layers = 1;

		vkCreateFramebuffer(context.device, &frameBufferInfo, nullptr, &rendercontext.framebuffers[i]);
	}
}


/////////////// IMAGE & TEXTURE IMAGE ///////////////
void VulkanGraphicsApplication::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = format; //must be same format as pixel in the buffer
	imageInfo.tiling = tiling;

	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;

	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	//imageInfo.flags = 0; //optional

	if (vkCreateImage(context.device, &imageInfo, nullptr, &image) != VK_SUCCESS) 
		throw std::runtime_error("failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(context.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = context.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(context.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) 
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(context.device, image, imageMemory, 0);
}

VkImageView VulkanGraphicsApplication::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo imgViewInfo = {};
	imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgViewInfo.image = image;
	imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imgViewInfo.format = format;
	imgViewInfo.subresourceRange.aspectMask = aspectFlags;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(context.device, &imgViewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");

	return imageView;
}

void VulkanGraphicsApplication::CreateTextureImage(const std::string path, VkImage& image, VkDeviceMemory& deviceMemory, VkFormat imageFormat)
{
	int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load("../textures/mytexture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); //direct path
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) 
		throw std::runtime_error("failed to load texture image!");
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(context.device, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(texWidth, texHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, deviceMemory);

	TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); //for shader access

	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

void VulkanGraphicsApplication::CreateTextureImageViews()
{
	rendercontext.albedoImageView = CreateImageView(rendercontext.albedoImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	rendercontext.normalImageView = CreateImageView(rendercontext.normalImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	rendercontext.metallicImageView = CreateImageView(rendercontext.metallicImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	rendercontext.roughnessImageView = CreateImageView(rendercontext.roughnessImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	rendercontext.aoImageView = CreateImageView(rendercontext.aoImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanGraphicsApplication::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 8.f; //limits the amount of texel samples that can be used to calculate the final color. Max 16.f

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;

	samplerInfo.unnormalizedCoordinates = VK_FALSE; //specifies which coordinate system you want to use to address texels in an image

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.albedoSampler) != VK_SUCCESS) 
		throw std::runtime_error("failed to create texture sampler!");

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.normalSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.metallicSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.roughnessSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &rendercontext.aoSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

void VulkanGraphicsApplication::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0; //specifies the byte offset in the buffer at which the pixel values start
	region.bufferRowLength = 0; //0 = the pixels are simply tightly packed like they are in our case
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage( commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}


void VulkanGraphicsApplication::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
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

	vkCmdPipelineBarrier(commandBuffer,
		sourceStage /*specifies in which pipeline stage the operations occur that should happen before the barrier*/,
		destinationStage /*specifies the pipeline stage in which operations will wait on the barrier*/,
		0 /*turns the barrier into a per-region condition*/,
		0, nullptr, //arrays of pipeline barriers of the three available types: memory barriers
		0, nullptr, //, buffer memory barriers
		1, &barrier); //and image memory barriers

	EndSingleTimeCommands(commandBuffer);
}

/////////////// MODEL //////////////////////
void VulkanGraphicsApplication::ComputeTangentBasis(std::vector<Vertex>& vertices)
{
	for (int i = 0; i < rendercontext.vertices.size(); i += 3) 
	{
		//shortcuts for vertices
		glm::vec3& v0 = rendercontext.vertices[i + 0].pos;
		glm::vec3& v1 = rendercontext.vertices[i + 1].pos;
		glm::vec3& v2 = rendercontext.vertices[i + 2].pos;

		//shortcuts for UVs
		glm::vec2& uv0 = rendercontext.vertices[i + 0].uv;
		glm::vec2& uv1 = rendercontext.vertices[i + 1].uv;
		glm::vec2& uv2 = rendercontext.vertices[i + 2].uv;

		//edges of the triangle : postion delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		//UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

		for (int j = 0; j < 3; j++)
		{
			rendercontext.vertices[i + j].tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
			rendercontext.vertices[i + j].bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
		}
	}
}

void VulkanGraphicsApplication::LoadModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) 
	{
		throw std::runtime_error(warn + err);
	}

	//std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.pos = 
			{
				attrib.vertices[3 * index.vertex_index + 0], //array of float instead of glm::vec3, so you need to multiply the index by 3
				attrib.vertices[3 * index.vertex_index + 1], //offsets of 0, 1 and 2 used to access the X, Y and Z components
				attrib.vertices[3 * index.vertex_index + 2]
			};

			//vertex.color = { 1.0f, 1.0f, 1.0f };

			/*vertex.norm =
			{
				attrib.vertices[3 * index.normal_index + 0], //array of float instead of glm::vec3, so you need to multiply the index by 3
				attrib.vertices[3 * index.normal_index + 1], //offsets of 0, 1 and 2 used to access the X, Y and Z components
				attrib.vertices[3 * index.normal_index + 2]
			};*/

			vertex.uv = 
			{
				attrib.texcoords[2 * index.texcoord_index + 0], //offsets of 0 and 1 used to access U and V components
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			rendercontext.vertices.push_back(vertex);
			rendercontext.indices.push_back(rendercontext.indices.size());
		}
	}
	ComputeTangentBasis(rendercontext.vertices);
}

/////////////// DEPTH BUFFER ///////////////
void VulkanGraphicsApplication::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	CreateImage(context.swapchainExtent.width, context.swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rendercontext.depthImage, rendercontext.depthImageMemory);
	rendercontext.depthImageView = CreateImageView(rendercontext.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat VulkanGraphicsApplication::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) 
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(context.physicalDevice, format, &props);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) || (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
			return format;

		throw std::runtime_error("failed to find supported format!");
	}
}

VkFormat VulkanGraphicsApplication::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

/*const std::vector<Vertex> vertices = { {{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, //3D SQUARE
										{{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
										{{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
										{{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

										{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
										{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
										{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
										{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}

									//  {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, //SQUARE
										{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
										{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
										{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}

									    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, //TRIANGLE
										{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
										{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} };*/

/*const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0, //SQUARE 1
										4, 5, 6, 6, 7, 4 }; //SQUARE 2*/


void VulkanGraphicsApplication::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(rendercontext.vertices[0]) * rendercontext.vertices.size();
	
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, rendercontext.stagingBuffer, rendercontext.stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, rendercontext.stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, rendercontext.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.device, rendercontext.stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rendercontext.vertexBuffer, rendercontext.vertexBufferMemory);

	CopyBuffer(rendercontext.stagingBuffer, rendercontext.vertexBuffer, bufferSize);

	vkDestroyBuffer(context.device, rendercontext.stagingBuffer, nullptr);
	vkFreeMemory(context.device, rendercontext.stagingBufferMemory, nullptr);
}

void VulkanGraphicsApplication::CreateIndexBuffer() 
{
	VkDeviceSize bufferSize = sizeof(rendercontext.indices[0]) * rendercontext.indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, rendercontext.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.device, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, rendercontext.indexBuffer, rendercontext.indexBufferMemory);

	CopyBuffer(stagingBuffer, rendercontext.indexBuffer, bufferSize);

	vkDestroyBuffer(context.device, stagingBuffer, nullptr);
	vkFreeMemory(context.device, stagingBufferMemory, nullptr);
}

////////////// UBO ///////////////////////
struct UniformBufferObject 
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
			    
	alignas(16) glm::vec3 lightPos;
	alignas(16) glm::vec3 lightCol;
};

struct Transform
{
	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
};

Transform camTrans;
Transform lightTrans;

void VulkanGraphicsApplication::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	rendercontext.uniformBuffers.resize(context.SWAPCHAIN_IMAGES);
	rendercontext.uniformBuffersMemory.resize(context.SWAPCHAIN_IMAGES);

	for (size_t i = 0; i < context.SWAPCHAIN_IMAGES; i++)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, rendercontext.uniformBuffers[i], rendercontext.uniformBuffersMemory[i]);
	}
}

void VulkanGraphicsApplication::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 6> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	//albedo
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	//normal
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	//metallic
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	//roughness
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[4].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	//ao
	poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[5].descriptorCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);

	VkDescriptorPoolCreateInfo descrPoolInfo = {};
	descrPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descrPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descrPoolInfo.pPoolSizes = poolSizes.data();
	//descrPoolInfo.flags = 0; //Optional
	descrPoolInfo.maxSets = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);

	if (vkCreateDescriptorPool(context.device, &descrPoolInfo, nullptr, &rendercontext.descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanGraphicsApplication::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//uboLayoutBinding.pImmutableSamplers = nullptr; //optional

	VkDescriptorSetLayoutBinding albedoLayoutBinding = {};
	albedoLayoutBinding.binding = 1;
	albedoLayoutBinding.descriptorCount = 1;
	albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedoLayoutBinding.pImmutableSamplers = nullptr;
	albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalLayoutBinding = {};
	normalLayoutBinding.binding = 2;
	normalLayoutBinding.descriptorCount = 1;
	normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalLayoutBinding.pImmutableSamplers = nullptr;
	normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding metallicLayoutBinding = {};
	metallicLayoutBinding.binding = 3;
	metallicLayoutBinding.descriptorCount = 1;
	metallicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	metallicLayoutBinding.pImmutableSamplers = nullptr;
	metallicLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding roughnessLayoutBinding = {};
	roughnessLayoutBinding.binding = 4;
	roughnessLayoutBinding.descriptorCount = 1;
	roughnessLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	roughnessLayoutBinding.pImmutableSamplers = nullptr;
	roughnessLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding aoLayoutBinding = {};
	aoLayoutBinding.binding = 5;
	aoLayoutBinding.descriptorCount = 1;
	aoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	aoLayoutBinding.pImmutableSamplers = nullptr;
	aoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 6> bindings = { uboLayoutBinding, albedoLayoutBinding, normalLayoutBinding, metallicLayoutBinding, roughnessLayoutBinding, aoLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr, &rendercontext.descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VulkanGraphicsApplication::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(context.SWAPCHAIN_IMAGES, rendercontext.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = rendercontext.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(context.SWAPCHAIN_IMAGES);
	allocInfo.pSetLayouts = layouts.data();

	rendercontext.descriptorSets.resize(context.SWAPCHAIN_IMAGES); //no need to clean up because will be freed when descrPool destroyed
	if (vkAllocateDescriptorSets(context.device, &allocInfo, rendercontext.descriptorSets.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < context.SWAPCHAIN_IMAGES; i++) 
	{
		VkDescriptorBufferInfo bufferInfo = {}; //overwrite of the whole buffer
		bufferInfo.buffer = rendercontext.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject); //or VK_WHOLE_SIZE

		VkDescriptorImageInfo albedoInfo = {};
		albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoInfo.imageView = rendercontext.albedoImageView;
		albedoInfo.sampler = rendercontext.albedoSampler;

		VkDescriptorImageInfo normalInfo = {};
		normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalInfo.imageView = rendercontext.normalImageView;
		normalInfo.sampler = rendercontext.normalSampler;

		VkDescriptorImageInfo metallicInfo = {};
		metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		metallicInfo.imageView = rendercontext.metallicImageView;
		metallicInfo.sampler = rendercontext.metallicSampler;

		VkDescriptorImageInfo roughnessInfo = {};
		roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		roughnessInfo.imageView = rendercontext.roughnessImageView;
		roughnessInfo.sampler = rendercontext.roughnessSampler;

		VkDescriptorImageInfo aoInfo = {};
		aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		aoInfo.imageView = rendercontext.aoImageView;
		aoInfo.sampler = rendercontext.aoSampler;

		std::array<VkWriteDescriptorSet, 6> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		/*descriptorWrite.pImageInfo = nullptr; //optional
		descriptorWrite.pTexelBufferView = nullptr; //optional*/

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &albedoInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &normalInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &metallicInfo;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[4].dstBinding = 4;
		descriptorWrites[4].dstArrayElement = 0;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[4].descriptorCount = 1;
		descriptorWrites[4].pImageInfo = &roughnessInfo;

		descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[5].dstSet = rendercontext.descriptorSets[i];
		descriptorWrites[5].dstBinding = 5;
		descriptorWrites[5].dstArrayElement = 0;
		descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[5].descriptorCount = 1;
		descriptorWrites[5].pImageInfo = &aoInfo;

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void VulkanGraphicsApplication::UpdateUniformBuffer(uint32_t currentImage)
{
	static std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();

	std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.f, 0.0f));

	/*ubo.view = glm::lookAt(glm::vec3(1.0f, 0.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //100 for StingSword & 3 for Rapier
	ubo.proj = glm::perspective(glm::radians(45.0f), context.swapchainExtent.width / (float)context.swapchainExtent.height, 0.1f, 10.0f); //1000 for StingSword
	ubo.proj[1][1] *= -1; //not the same plan as OpenGL. Here to adapt*/

	ubo.view = glm::lookAt(camTrans.position, camTrans.position + glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), context.swapchainExtent.width / (float)context.swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	ubo.lightPos = lightTrans.position;
	ubo.lightCol = lightTrans.scale;

	void* data;
	vkMapMemory(context.device, rendercontext.uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(context.device, rendercontext.uniformBuffersMemory[currentImage]);
}

////////////// SHADERS //////////////////
static std::vector<char> readFile(const std::string& filename) 
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) 
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void VulkanGraphicsApplication::CreateGraphicsPipeline()
{
	std::vector<char> vertShaderCode = readFile("../shaders/PBvert.spv");
	std::vector<char> fragShaderCode = readFile("../shaders/PBfrag.spv");

	VkShaderModule vertShaderModule = context.CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = context.CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 5Ui64> attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)context.swapchainExtent.width;
	viewport.height = (float)context.swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	/*rasterizer.depthBiasConstantFactor = 0.0f; //optional
	rasterizer.depthBiasClamp = 0.0f; //optional
	rasterizer.depthBiasSlopeFactor = 0.0f; //optional*/

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	/*multisampling.minSampleShading = 1.0f; //optional
	multisampling.pSampleMask = nullptr; //optional
	multisampling.alphaToCoverageEnable = VK_FALSE; //optional
	multisampling.alphaToOneEnable = VK_FALSE; //optional*/

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE; //specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded
	depthStencil.depthWriteEnable = VK_TRUE; //specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; //specifies the comparison that is performed to keep or discard fragments.
	//here convention of lower depth = closer, so the depth of new fragments should be less
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	/*depthStencil.minDepthBounds = 0.0f; //optional
	depthStencil.maxDepthBounds = 1.0f; //optional*/
	depthStencil.stencilTestEnable = VK_FALSE;
	//depthStencil.front = {}; //optional
	//depthStencil.back = {}; //optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	/*colorBlending.logicOp = VK_LOGIC_OP_COPY; //optional
	colorBlending.blendConstants[0] = 0.0f; //optional
	colorBlending.blendConstants[1] = 0.0f; //optional
	colorBlending.blendConstants[2] = 0.0f; //optional
	colorBlending.blendConstants[3] = 0.0f; //optional*/

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &rendercontext.descriptorSetLayout;
	/*pipelineLayoutInfo.pushConstantRangeCount = 0; //optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; //optional*/

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &rendercontext.graphicsPipelineLayout) != VK_SUCCESS) 
		throw std::runtime_error("failed to create pipeline layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; //must always be specified if the render pass contains a depth stencil attachment.
	pipelineInfo.pColorBlendState = &colorBlending;
	/*pipelineInfo.pDepthStencilState = nullptr; //optional
	pipelineInfo.pDynamicState = nullptr; //optional*/

	pipelineInfo.layout = rendercontext.graphicsPipelineLayout;
	pipelineInfo.renderPass = rendercontext.renderPass;
	pipelineInfo.subpass = 0;
	/*pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optionalVkFramebufferCreateInfo
	pipelineInfo.basePipelineIndex = -1; //optional*/

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rendercontext.graphicsPipeline) != VK_SUCCESS) 
		throw std::runtime_error("failed to create graphics pipeline!");

	vkDestroyShaderModule(context.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertShaderModule, nullptr);
}

bool VulkanGraphicsApplication::Prepare()
{
	//Create semaphores -> loop to SWAPCHAIN
	VkSemaphoreCreateInfo semCreateInfo = {};
	semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++)
	{
		DEBUG_CHECK_VK(vkCreateSemaphore(context.device, &semCreateInfo, nullptr, &context.acquireSemaphores[i]));
		DEBUG_CHECK_VK(vkCreateSemaphore(context.device, &semCreateInfo, nullptr, &context.presentSemaphores[i]));
	}
	//Create command buffer pool
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	DEBUG_CHECK_VK(vkCreateCommandPool(context.device, &cmdPoolCreateInfo, nullptr, &rendercontext.mainCommandPool));

	//Create fences
	VkFenceCreateInfo fenCreateInfo = {};
	fenCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//Create command buffer (1 per frame)
	VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
	cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocInfo.commandPool = rendercontext.mainCommandPool;
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	for (int i = 0; i < rendercontext.PENDING_FRAMES; i++)
	{
		//Create fences SIGNALED STATE
		DEBUG_CHECK_VK(vkCreateFence(context.device, &fenCreateInfo, nullptr, &rendercontext.mainFences[i]));
		DEBUG_CHECK_VK(vkAllocateCommandBuffers(context.device, &cmdBufferAllocInfo, &rendercontext.mainCommandBuffers[i]));
	}

	//Create the render pass
	VkAttachmentDescription colAttDescript = {};
	colAttDescript.format = context.surfaceFormat.format;
	colAttDescript.samples = VK_SAMPLE_COUNT_1_BIT;
	colAttDescript.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colAttDescript.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colAttDescript.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colAttDescript.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat(); //should be the same as the depth image itself
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colAttRef = {};
	colAttRef.attachment = 0; //only 1, ID 0
	colAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescript = {};
	subpassDescript.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescript.colorAttachmentCount = 1;
	subpassDescript.pColorAttachments = &colAttRef;
	subpassDescript.pDepthStencilAttachment = &depthAttachmentRef;

	/*VkSubpassDependency subPassDependency = {};
	subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDependency.dstSubpass = 0;
	subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDependency.srcAccessMask = 0;
	subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;*/

	std::array<VkAttachmentDescription, 2> attachmentsDescr = { colAttDescript, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentsDescr.size());
	renderPassInfo.pAttachments = attachmentsDescr.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescript;
	//renderPassInfo.dependencyCount = 1;
	//renderPassInfo.pDependencies = &subPassDependency;

	vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &rendercontext.renderPass);

	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++)
		context.swapchainImageViews[i] = CreateImageView(context.swapchainImages[i], context.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateDepthResources();
	CreateFrameBuffers();
	CreateTextureImage("../textures/Steel_albedo.png", rendercontext.albedoImage, rendercontext.albedoImageMemory, VK_FORMAT_R8G8B8A8_SRGB);
	CreateTextureImage("../textures/Steel_normal.png", rendercontext.normalImage, rendercontext.normalImageMemory, VK_FORMAT_R8G8B8A8_UNORM);
	CreateTextureImage("../textures/Steel_metallic.png", rendercontext.metallicImage, rendercontext.metallicImageMemory, VK_FORMAT_R8G8B8A8_UNORM);
	CreateTextureImage("../textures/Steel_roughness.png", rendercontext.roughnessImage, rendercontext.roughnessImageMemory, VK_FORMAT_R8G8B8A8_UNORM);
	CreateTextureImage("../textures/Steel_ao.png", rendercontext.aoImage, rendercontext.aoImageMemory, VK_FORMAT_R8G8B8A8_UNORM);
	CreateTextureImageViews();
	CreateTextureSampler();
	LoadModel();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();

	return true;
}

void VulkanGraphicsApplication::Terminate()
{
	//destroying the command buffer pool destroys the command buffer
	vkDestroyPipeline(context.device, rendercontext.graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(context.device, rendercontext.graphicsPipelineLayout, nullptr);

	vkDestroyRenderPass(context.device, rendercontext.renderPass, nullptr);

	for (int i = 0; i < context.SWAPCHAIN_IMAGES; i++)
	{
		vkDestroyImageView(context.device, context.swapchainImageViews[i], nullptr);
		vkDestroyFramebuffer(context.device, rendercontext.framebuffers[i], nullptr);

		vkDestroySemaphore(context.device, context.acquireSemaphores[i], nullptr);
		vkDestroySemaphore(context.device, context.presentSemaphores[i], nullptr);

		vkDestroyBuffer(context.device, rendercontext.uniformBuffers[i], nullptr);
		vkFreeMemory(context.device, rendercontext.uniformBuffersMemory[i], nullptr);
	}
	
	vkDestroyDescriptorPool(context.device, rendercontext.descriptorPool, nullptr);

	vkDestroySampler(context.device, rendercontext.aoSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.aoImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.aoImage, nullptr);
	vkFreeMemory(context.device, rendercontext.aoImageMemory, nullptr);

	vkDestroySampler(context.device, rendercontext.roughnessSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.roughnessImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.roughnessImage, nullptr);
	vkFreeMemory(context.device, rendercontext.roughnessImageMemory, nullptr);

	vkDestroySampler(context.device, rendercontext.metallicSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.metallicImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.metallicImage, nullptr);
	vkFreeMemory(context.device, rendercontext.metallicImageMemory, nullptr);

	vkDestroySampler(context.device, rendercontext.normalSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.normalImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.normalImage, nullptr);
	vkFreeMemory(context.device, rendercontext.normalImageMemory, nullptr);

	vkDestroySampler(context.device, rendercontext.albedoSampler, nullptr);
	vkDestroyImageView(context.device, rendercontext.albedoImageView, nullptr);
	vkDestroyImage(context.device, rendercontext.albedoImage, nullptr);
	vkFreeMemory(context.device, rendercontext.albedoImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(context.device, rendercontext.descriptorSetLayout, nullptr);

	vkDestroyBuffer(context.device, rendercontext.indexBuffer, nullptr);
	vkFreeMemory(context.device, rendercontext.indexBufferMemory, nullptr);
	vkDestroyBuffer(context.device, rendercontext.vertexBuffer, nullptr);
	vkFreeMemory(context.device, rendercontext.vertexBufferMemory, nullptr);

	for (int i = 0; i < rendercontext.PENDING_FRAMES; i++)
		vkDestroyFence(context.device, rendercontext.mainFences[i], nullptr);

	vkDestroyCommandPool(context.device, rendercontext.mainCommandPool, nullptr);
}

//
// Frame
//

bool VulkanGraphicsApplication::Begin()
{
	uint64_t timeout = UINT64_MAX; //undefined waiting
	/*properly use m_currentFrame as fence & commandbuffer index
	Wait&Reset of the fence*/
	VkResult res;
	res = vkWaitForFences(context.device, 1, &rendercontext.mainFences[m_currentFrame], VK_FALSE, timeout);
	vkResetFences(context.device, 1, &rendercontext.mainFences[m_currentFrame]);
	/*we know the commandbuffer can be modified without impactig frame N-2
	Reset command buffer*/
	vkResetCommandBuffer(rendercontext.mainCommandBuffers[m_currentFrame], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	DEBUG_CHECK_VK(vkAcquireNextImageKHR(context.device, context.swapchain, timeout, context.acquireSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex));

	UpdateUniformBuffer(m_currentFrame);

	glfwSetKeyCallback(window, KeyCallback);

	return true;
}

bool VulkanGraphicsApplication::End()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//WAIT to draw color attachment before signal
	VkPipelineStageFlags stageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = stageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &rendercontext.mainCommandBuffers[m_currentFrame];
	//once the commandbuffer is executed, we signal the semaphore handling the presentation
	//the presentation may be executed on another queue
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &context.presentSemaphores[m_imageIndex];

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &context.acquireSemaphores[m_imageIndex];
	//vkQueueSubmit indicates which fence will be signaled - the one we're waiting with WaitForFences
	vkQueueSubmit(rendercontext.graphicsQueue, 1, &submitInfo, rendercontext.mainFences[m_currentFrame]);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;		//OFF, pas forcement necessaire ici //1 quand on aura un semaphore
	presentInfo.pWaitSemaphores = &context.presentSemaphores[m_imageIndex];//presentation semaphore ici synchro avec le dernier vkQueueSubmit;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &context.swapchain;
	presentInfo.pImageIndices = &m_imageIndex;
	DEBUG_CHECK_VK(vkQueuePresentKHR(rendercontext.presentQueue, &presentInfo));
	vkQueueWaitIdle(rendercontext.presentQueue);

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

	lightTrans.scale += (ctrl.lightIntensity * deltaTime * ctrl.speed);

	return true;
}

bool VulkanGraphicsApplication::Display()
{
	//"begin" commandbuffer
	VkCommandBuffer cmdBuffer = rendercontext.mainCommandBuffers[m_currentFrame];
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo);

	/*VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.layerCount = 1;
	range.levelCount = 1; //LVL of details = 1 because no mipmap created
	VkClearColorValue clear = { 1.f, 1.f, 0.f, 1.f };
	//Clear screen
	vkCmdClearColorImage(cmdBuffer, context.swapchainImages[m_imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clear, 1, &range);*/

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.framebuffer = rendercontext.framebuffers[m_currentFrame];
	renderPassBeginInfo.renderPass = rendercontext.renderPass;
	renderPassBeginInfo.renderArea.extent = context.swapchainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.1f, 0.f, 0.3f, 1.f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rendercontext.graphicsPipeline);

	VkBuffer vertexBuffers[] = { rendercontext.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(cmdBuffer, rendercontext.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rendercontext.graphicsPipelineLayout, 0, 1, &rendercontext.descriptorSets[m_currentFrame], 0, nullptr);

	vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(rendercontext.indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	//"end" command buffer
	vkEndCommandBuffer(cmdBuffer);

	return true;
}

int main(void)
{
	//initialize the library
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	VulkanGraphicsApplication app;

	//create a windowed mode window and its OpenGL context
	app.window = glfwCreateWindow(1024, 768, APP_NAME, NULL, NULL);
	if (!app.window)
	{
		glfwTerminate();
		return -1;
	}

	app.Initialize(APP_NAME);

	camTrans.position = glm::vec3(0.0f, 0.3f, 1.f);
	lightTrans.position = glm::vec3(0.f, 0.3f, 1.f);
	lightTrans.scale = glm::vec3(2.347f, 2.131f, 2.079f);

	//loop until the user closes the window
	while (!glfwWindowShouldClose(app.window))
	{
		//render here
		app.Run();

		//poll for and process events
		glfwPollEvents();
		
	}

	app.Shutdown();

	glfwTerminate();
	return 0;
}
