#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;
class PhysicalDevice;
class CommandPool;
class RenderPass;
class SwapChain;

class Buffer2D
{
public:
	Buffer2D(LogicalDevice* pCpu, CommandPool* pCommandPool, RenderPass* pRenderPass, PhysicalDevice* pGpu, uint32_t width, uint32_t height, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags flags, VkFormat format, VkImageAspectFlagBits aspectFlagBits, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
	~Buffer2D();

	const VkImage& GetImage() const { return m_Image; }
	const VkDeviceMemory& GetImageMemory() const { return m_ImageMemory; }
	const VkImageView& GetImageView() const { return m_ImageView; }

private:

	//Trifecta of resources
	VkImage m_Image;
	VkDeviceMemory m_ImageMemory;
	VkImageView m_ImageView;

	LogicalDevice* m_pCpu;
};