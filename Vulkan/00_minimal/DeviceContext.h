#pragma once

#include "vk_common.h"

struct VulkanDeviceContext
{
	static const int MAX_DEVICE_COUNT = 9;	// arbitraire, max = IGP + 2x4 GPU max
	static const int MAX_FAMILY_COUNT = 4;	// graphics, compute, transfer, graphics+compute (ajouter sparse aussi...)
	static const int SWAPCHAIN_IMAGES = 2;

	VkDebugReportCallbackEXT debugCallback;

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkSurfaceKHR surface;
	VkSurfaceFormatKHR surfaceFormat;
	VkSwapchainKHR swapchain;
	VkExtent2D swapchainExtent;
	VkImage swapchainImages[SWAPCHAIN_IMAGES];
	VkImageView swapchainImageViews[SWAPCHAIN_IMAGES];
	VkSemaphore acquireSemaphores[SWAPCHAIN_IMAGES];
	VkSemaphore presentSemaphores[SWAPCHAIN_IMAGES];

	VkPhysicalDeviceProperties props;
	std::vector<VkMemoryPropertyFlags> memoryFlags;

	VkShaderModule CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
			std::cout << "failed to create shader module!" << std::endl;

		return shaderModule;
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
	{
		/*int i = 0;
		for (auto propertyFlags : memoryFlags) 
		{
			if ((typeFilter & (1 << i)) && (propertyFlags & properties) == properties) //bitwise operation
			{
				return i;
			}
			i++;
		}

		std::cout << "failed to find suitable memory type!" << std::endl;
		return 0;
	}*/

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
				return i;
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
};

