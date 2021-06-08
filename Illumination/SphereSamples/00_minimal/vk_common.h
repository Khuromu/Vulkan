#pragma once

#if defined(_DEBUG)
#define VULKAN_ENABLE_VALIDATION
#endif

#include "volk/volk.h"

#ifdef _DEBUG
#define DEBUG_CHECK_VK(x) if (VK_SUCCESS != (x)) { std::cout << (#x) << std::endl; __debugbreak(); }
#else
#define DEBUG_CHECK_VK(x) 
#endif

//#define OPENGL_NDC

#define GLM_FORCE_RADIANS
#ifndef OPENGL_NDC
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb/stb_image.h"

enum PixelFormat
{
	PIXFMT_RGBA8,
	PIXFMT_SRGBA8,
	PIXFMT_RGBA16F,
	PIXFMT_RGBA32F,
	PIXFMT_DUMMY_ASPECT_DEPTH,
	PIXFMT_DEPTH32F = PIXFMT_DUMMY_ASPECT_DEPTH,
	MAX
};

enum ImageUsageBits
{
	IMAGE_USAGE_TEXTURE = 1 << 0,
	IMAGE_USAGE_BITMAP = 1 << 1,
	IMAGE_USAGE_RENDERTARGET = 1 << 2,
	IMAGE_USAGE_RENDERPASS = 1 << 6,
	IMAGE_USAGE_STAGING = 1 << 7
};
typedef uint32_t ImageUsage;

#include <iostream>
#include <fstream>
#include <vector>
#include <array>

