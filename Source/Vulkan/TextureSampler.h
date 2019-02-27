#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;

class TextureSampler
{
public:
	TextureSampler(LogicalDevice* pCpu, uint32_t mipLevels);
	~TextureSampler();

	const VkSampler& GetSampler() const { return m_Sampler; }

private:
	VkSampler m_Sampler;

	LogicalDevice* m_pCpu;

};