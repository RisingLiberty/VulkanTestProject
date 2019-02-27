#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class SwapChain;
class LogicalDevice;
class PhysicalDevice;

class RenderPass
{
public:
	RenderPass(LogicalDevice* pDevice, SwapChain* pSwapchain, PhysicalDevice* pGpu);
	~RenderPass();

	void SetSamplesCount(VkSampleCountFlagBits msaaSamples) { m_msaaSamples = msaaSamples; }

	const VkRenderPass& GetRenderPass() const { return m_RenderPass; }
	VkSampleCountFlagBits GetSamplesCount() const { return m_msaaSamples; }

private:
	VkRenderPass m_RenderPass;
	LogicalDevice* m_pDevice;
	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
};