#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

class LogicalDevice;
class PhysicalDevice;

class CommandPool
{
public: 
	CommandPool(LogicalDevice* pCpu, PhysicalDevice* pGpu);
	~CommandPool();

	const VkCommandPool& GetPool() const { return m_CommandPool; }

private:
	VkCommandPool m_CommandPool;
	LogicalDevice* m_pCpu;
};
