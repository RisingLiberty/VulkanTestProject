#pragma once

//GLFW Defines and includes
#ifndef	GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;

class Semaphore
{
public:
	Semaphore(LogicalDevice* pCpu);
	~Semaphore();

	const VkSemaphore& GetSemaphore() const { return m_Semaphore; }

private:
	VkSemaphore m_Semaphore;
	LogicalDevice* m_pCpu;
};