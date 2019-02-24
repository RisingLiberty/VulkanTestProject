#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;
class PhysicalDevice;

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, LogicalDevice* pLogicalDevice);
void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, LogicalDevice* pLogicalDevice, PhysicalDevice* pGpu);
uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, PhysicalDevice* pGpu);
VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, PhysicalDevice* pGpu);
VkFormat FindDepthFormat(PhysicalDevice* pGpu);
void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const VkCommandPool& commandPool, LogicalDevice* pCpu);
VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool& commandPool, LogicalDevice* pCpu);
void EndSingleTimeCommands(VkCommandBuffer commandBuffer, const VkCommandPool& commandPool, LogicalDevice* pCpu);
void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, LogicalDevice* pCpu, PhysicalDevice* pGpu);
void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const VkCommandPool& commandPool, LogicalDevice* pCpu);
bool HasStencilComponent(VkFormat format);

