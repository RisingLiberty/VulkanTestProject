#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;
class PhysicalDevice;
class CommandPool;

class Texture
{
public: 
	Texture(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandPool);
	~Texture();

	const VkImage& GetImage() const { return m_Texture; }
	const VkDeviceMemory& GetMemory() const { return m_Memory; }
	const VkImageView& GetImageView() const { return m_TextureView; }
	const uint32_t GetSamples() const { return m_MipLevels; }

private:
	void GenerateMipMaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, PhysicalDevice* pGpu, CommandPool* pCommandPool);

private:
	VkImage m_Texture;
	VkDeviceMemory m_Memory;
	VkImageView m_TextureView;

	LogicalDevice* m_pCpu;
	uint32_t m_MipLevels;
};