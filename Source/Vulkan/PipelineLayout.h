#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

class LogicalDevice;
class DescriptorSetLayout;

class PipelineLayout
{
public:
	PipelineLayout(LogicalDevice* pCpu, DescriptorSetLayout* pDescSetLayout);

	const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

private:
	VkPipelineLayout m_PipelineLayout;
};