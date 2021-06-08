#include "vk_common.h"

// A definir en premier lieu
#if defined(_WIN32)
// a definir globalement pour etre pris en compte par volk.c
//#define VK_USE_PLATFORM_WIN32_KHR

// si glfw
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#elif defined(_GNUC_)
#endif

#if defined(_MSC_VER)
//
// Pensez a copier les dll dans le repertoire x64/Debug, cad:
// glfw-3.3/lib-vc2015/glfw3.dll
//
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "vulkan-1.lib")
#endif

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "GraphicsApplication.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cout << "[VULKAN VALIDATION]" << msg << std::endl;
#ifdef _WIN32
	if (IsDebuggerPresent()) {
		OutputDebugStringA("[VULKAN VALIDATION] ");
		OutputDebugStringA(msg);
		OutputDebugStringA("\n");
	}
#endif
	return VK_FALSE;
}

bool VulkanGraphicsApplication::Initialize(const char* appName)
{
	name = appName;

	// Vulkan

	DEBUG_CHECK_VK(volkInitialize());

	// instance

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "todo engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	const char *extensionNames[] = { VK_KHR_SURFACE_EXTENSION_NAME
#if defined(_WIN32)
		, VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
		, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = sizeof(extensionNames) / sizeof(char*);
	instanceInfo.ppEnabledExtensionNames = extensionNames;
#ifdef VULKAN_ENABLE_VALIDATION
	const char* layerNames[] = { "VK_LAYER_KHRONOS_validation" };
	instanceInfo.enabledLayerCount = 1;
	instanceInfo.ppEnabledLayerNames = layerNames;
#else
	instanceInfo.enabledExtensionCount--;
#endif
	DEBUG_CHECK_VK(vkCreateInstance(&instanceInfo, nullptr, &context.instance));
	// TODO: fallback si pas de validation possible (MoltenVK, toujours le cas ?)

	volkLoadInstance(context.instance);

#ifdef VULKAN_ENABLE_VALIDATION
	VkDebugReportCallbackCreateInfoEXT debugCallbackInfo = {};
	debugCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugCallbackInfo.flags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	debugCallbackInfo.pfnCallback = VulkanReportFunc;
	vkCreateDebugReportCallbackEXT(context.instance, &debugCallbackInfo, nullptr, &context.debugCallback);
#endif

	// render surface

#if defined(USE_GLFW_SURFACE)
	glfwCreateWindowSurface(g_Context.instance, g_Context.window, nullptr, &g_Context.surface);
#else
#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hinstance = GetModuleHandle(NULL);
	surfaceInfo.hwnd = glfwGetWin32Window(window);
	DEBUG_CHECK_VK(vkCreateWin32SurfaceKHR(context.instance, &surfaceInfo, nullptr, &context.surface));
#endif
#endif

	// device

	uint32_t num_devices = context.MAX_DEVICE_COUNT;
	std::vector<VkPhysicalDevice> physical_devices(num_devices);
	DEBUG_CHECK_VK(vkEnumeratePhysicalDevices(context.instance, &num_devices, &physical_devices[0]));

	context.physicalDevice = physical_devices[0];

	rendercontext.graphicsQueueIndex = UINT32_MAX;
	uint32_t queue_families_count = context.MAX_FAMILY_COUNT;
	std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);
	// normalement il faut appeler la fonction une premiere fois pour recuperer le nombre exact de queues supportees.
	//voir les messages de validation
	vkGetPhysicalDeviceQueueFamilyProperties(context.physicalDevice, &queue_families_count, &queue_family_properties[0]);
	for (uint32_t i = 0; i < queue_families_count; ++i) {
		if ((queue_family_properties[i].queueCount > 0) &&
			(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			VkBool32 canPresentSurface;
			vkGetPhysicalDeviceSurfaceSupportKHR(context.physicalDevice, i, context.surface, &canPresentSurface);
			if (canPresentSurface)
				rendercontext.graphicsQueueIndex = i;
			break;
		}
	}

	// on suppose que la presentation se fait par la graphics queue (verifier cela avec vkGetPhysicalDeviceSurfaceSupportKHR())
	rendercontext.presentQueueIndex = rendercontext.graphicsQueueIndex;

	const float queue_priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = rendercontext.graphicsQueueIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queue_priorities;

	//SAMPLER//
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = device_extensions;
	DEBUG_CHECK_VK(vkCreateDevice(context.physicalDevice, &deviceInfo, nullptr, &context.device));

	volkLoadDevice(context.device);

	vkGetDeviceQueue(context.device, rendercontext.graphicsQueueIndex, 0, &rendercontext.graphicsQueue);
	rendercontext.presentQueue = rendercontext.graphicsQueue;

	// swap chain

	// todo: enumerer (cf validation)
	uint32_t formatCount = 16;
	VkSurfaceFormatKHR surfaceFormats[16];
	vkGetPhysicalDeviceSurfaceFormatsKHR(context.physicalDevice, context.surface, &formatCount, surfaceFormats);
	// on sait que le moniteur applique une conversion implicite de sRGB vers lineaire
	// mais nos shader ecrivent du lineaire (nos calculs sont lineaires)
	// il faut donc forcer l'ecriture dans le framebuffer en sRGB
	// VKFORMAT***_SRGB -> conversion automatique lineaire vers sRGB
	// attention ce changement ne concerne que la swapchain ici
	for (unsigned int i = 0; i < formatCount; i++) 
	{
		if (surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
			surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) 
		{
			context.surfaceFormat = surfaceFormats[i];
			break;
		}
	}
	if (context.surfaceFormat.format == VK_FORMAT_UNDEFINED)
		__debugbreak();

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;   // FIFO est toujours garanti.
	uint32_t swapchainImageCount = context.SWAPCHAIN_IMAGES;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physicalDevice, context.surface, &surfaceCapabilities);
	context.swapchainExtent = surfaceCapabilities.currentExtent;
	VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // garanti
	//if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	//	imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // necessaire ici pour vkCmdClearImageColor
													   //	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
													   //		imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // necessaire ici pour screenshots, read back

	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = context.surface;
	swapchainInfo.minImageCount = swapchainImageCount;
	swapchainInfo.imageFormat = context.surfaceFormat.format;
	swapchainInfo.imageColorSpace = context.surfaceFormat.colorSpace;
	swapchainInfo.imageExtent = context.swapchainExtent;
	swapchainInfo.imageArrayLayers = 1; // 2 for stereo
	swapchainInfo.imageUsage = imageUsage;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	DEBUG_CHECK_VK(vkCreateSwapchainKHR(context.device, &swapchainInfo, nullptr, &context.swapchain));

	vkGetSwapchainImagesKHR(context.device, context.swapchain, &swapchainImageCount, context.swapchainImages);

	m_frame = 0;
	m_currentFrame = 0;

	return Prepare();
}

bool VulkanGraphicsApplication::Shutdown()
{
	if (context.instance == VK_NULL_HANDLE)
		return false;
	if (context.device == VK_NULL_HANDLE)
		return false;

	vkDeviceWaitIdle(context.device);

	Terminate();
	
	vkDestroySwapchainKHR(context.device, context.swapchain, nullptr);

	vkDestroyDevice(context.device, nullptr);

	vkDestroySurfaceKHR(context.instance, context.surface, nullptr);

#ifdef VULKAN_ENABLE_VALIDATION
	vkDestroyDebugReportCallbackEXT(context.instance, context.debugCallback, nullptr);
#endif

	vkDestroyInstance(context.instance, nullptr);

#ifndef GLFW_INCLUDE_VULKAN

#endif

	return true;
}

bool VulkanGraphicsApplication::Run()
{
	Update();
	Begin();
	Display();
	End();

	return true;
}
