#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

class PhysicalDevice;
class VulkanInstance;

class LogicalDevice
{
public:
	LogicalDevice(VulkanInstance* pInstance, PhysicalDevice* pGpu, const std::vector<const char*>& extensions, const std::vector<const char*>& validationLayers);
	~LogicalDevice();


	VkDevice GetDevice() const { return m_Device; }

	VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue GetPresentQueue() const { return m_PresentQueue; }

private:
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

};