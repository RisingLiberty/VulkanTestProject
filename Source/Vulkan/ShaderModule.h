#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <vector>

class LogicalDevice;

class ShaderModule
{
public:
	ShaderModule(LogicalDevice* pCpu, const std::vector<char>& code);
	~ShaderModule();
	const VkShaderModule& GetModule() const { return m_Module; }

private:
	VkShaderModule m_Module;
	LogicalDevice* m_pCpu;
};