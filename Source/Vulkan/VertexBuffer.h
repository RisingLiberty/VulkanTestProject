#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

#include "../Vulkan/Vertex.h"

class LogicalDevice;
class CommandPool;
class PhysicalDevice;

class VertexBuffer
{
public: 
	VertexBuffer(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandPool, const std::vector<Vertex>& vertices);
	~VertexBuffer();

	const VkBuffer& GetBuffer() const { return m_Buffer; }
	const VkDeviceMemory& GetBufferMemory() const { return m_BufferMemory; }

private:
	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;

	LogicalDevice* m_pCpu;
};