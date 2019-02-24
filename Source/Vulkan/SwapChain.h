#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vector>

class PhysicalDevice;
class Window;
class Surface;
class LogicalDevice;

//Exposes functions that can be used to generate model transformations like glm::rotate, 
//view transformations like glm::lookat
//proj transformation like glm::perspective.
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject
{
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Proj;
};

class SwapChain
{
public:
	SwapChain(PhysicalDevice* pPhysicalDevice, Window* pWindow, Surface* pSurface, LogicalDevice* pCpu);
	~SwapChain();

	VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
	VkExtent2D GetExtent() const { return m_SwapChainExtent; }
	VkFormat GetFormat() const { return m_SwapChainImageFormat; }
	const std::vector<VkFramebuffer>& GetFrameBuffers() const { return m_FrameBuffers; }
	const std::vector<VkImageView>& GetImageViews() const { return m_SwapChainImageViews; }
	const std::vector<VkImage>& GetImages() const { return m_SwapChainImages; }
	const std::vector<VkBuffer>& GetUniformBuffers() const { return m_UniformBuffers; }

	void CreateImageViews();
	void CreateFrameBuffers(const VkRenderPass& renderPass, const VkImageView& colorImageView, const VkImageView& depthImageView);
	void UpdateUniformBuffer(uint32_t currentImage);
	void CreateUniformBuffer();
private:
	VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	Window* m_pWindow;

	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
	std::vector<VkImageView> m_SwapChainImageViews;
	LogicalDevice* m_pLogicalDevice;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	std::vector<VkFramebuffer> m_FrameBuffers;

	uint32_t m_CurrentImage;

	PhysicalDevice* m_pPhysicalDevice;
};