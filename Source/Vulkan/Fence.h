#pragma once

//GLFW Defines and includes
#ifndef	GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;

class Fence
{
public:
	Fence(LogicalDevice* pCpu);
	~Fence();

	const VkFence& GetFence() const { return m_Fence; }

private:
	VkFence m_Fence;
	LogicalDevice* m_pCpu;
};