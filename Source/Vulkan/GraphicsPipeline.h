#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <memory>

class LogicalDevice;
class SwapChain;
class RenderPass;
class DescriptorSetLayout;
class PipelineLayout;

class GraphicsPipeline
{
public: 
	GraphicsPipeline(LogicalDevice* pCpu, SwapChain* pSwapChain, RenderPass* pRenderPass, DescriptorSetLayout* pDescSetLayout);
	~GraphicsPipeline();

	const VkPipeline& GetPipeline() const { return m_Pipeline; }
	PipelineLayout* GetLayout() const { return m_UniqueLayout.get(); }

private:
	VkPipeline m_Pipeline;
	LogicalDevice* m_pCpu;
	std::unique_ptr<PipelineLayout> m_UniqueLayout;
};