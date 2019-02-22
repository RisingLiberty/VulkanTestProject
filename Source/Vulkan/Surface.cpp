#include "Surface.h"

#include <stdexcept>

Surface::Surface(const VkInstance& instance, GLFWwindow* pWindow):
	m_Instance(instance)
{
	if (glfwCreateWindowSurface(instance, pWindow, nullptr, &m_Surface) != VK_SUCCESS)
		std::runtime_error("failed to create window surface!");
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}