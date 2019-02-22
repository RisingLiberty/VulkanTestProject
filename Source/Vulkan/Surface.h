#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

class Surface
{
public:
	Surface(const VkInstance& instance, GLFWwindow* pWindow);
	~Surface();

	VkSurfaceKHR GetSurface() const { return m_Surface; }

private:
	VkSurfaceKHR m_Surface;
	VkInstance m_Instance;
};