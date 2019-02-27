#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class LogicalDevice;
class DescriptorSetLayout;

class PipelineLayout
{
public:
	PipelineLayout(LogicalDevice* pCpu, DescriptorSetLayout* pDescSetLayout);
	~PipelineLayout();

	const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

private:
	VkPipelineLayout m_PipelineLayout;
	LogicalDevice* m_pCpu;
};