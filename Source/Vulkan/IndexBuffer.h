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

class IndexBuffer
{
public:
	IndexBuffer(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandBuffer, const std::vector<uint32_t>& indices);
	~IndexBuffer();
	
	const VkBuffer& GetBuffer() const { return m_Buffer; }
	const VkDeviceMemory GetBufferMemory() const { return m_BufferMemory; }
	size_t GetNrOfIndices() const { return m_NrOfIndices; }

private:
	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;
	size_t m_NrOfIndices;
	LogicalDevice* m_pCpu;
};