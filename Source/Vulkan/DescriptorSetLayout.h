#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(LogicalDevice* pCpu);
	~DescriptorSetLayout();

	const VkDescriptorSetLayout& GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

private:
	VkDescriptorSetLayout m_DescriptorSetLayout;
	LogicalDevice* m_pCpu;
};