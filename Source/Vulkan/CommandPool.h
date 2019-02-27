#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

class LogicalDevice;
class PhysicalDevice;
class RenderPass;
class SwapChain;
class VertexBuffer;
class IndexBuffer;
class GraphicsPipeline;

class CommandPool
{
public: 
	CommandPool(LogicalDevice* pCpu, PhysicalDevice* pGpu);
	~CommandPool();

	void CreateCommandBuffers(RenderPass* pRenderPass, SwapChain* pSwapChain, VertexBuffer* pVertexBuffer, IndexBuffer* pIndexBuffer, GraphicsPipeline* pGraphicsPipeline, const std::vector<VkDescriptorSet>& descriptorSets);

	const VkCommandPool& GetPool() const { return m_CommandPool; }
	const std::vector<VkCommandBuffer>& GetBuffers() const { return m_CommandBuffers; }

private:
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	LogicalDevice* m_pCpu;
};
