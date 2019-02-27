#include "IndexBuffer.h"

#include "LogicalDevice.h"
#include "CommandPool.h"

#include "../Help/HelperMethods.h"

IndexBuffer::IndexBuffer(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandPool, const std::vector<uint32_t>& indices):
	m_NrOfIndices(indices.size()),
	m_pCpu(pCpu)
{
	//There are only 2 notable idfferences. The bufferSize is now equal to the number of indices times the size of the index type,
	//either uint16_t or uint32_t. The usage of the m_IndexBuffer should be VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead
	//of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, which makes sense. Other than that, the process is exactly the same.
	//We create a staging buffer to copy the contents of m_Indices to and then copy it to the final device local index buffer.
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pCpu, pGpu);

	void* data;
	vkMapMemory(pCpu->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(pCpu->GetDevice(), stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory, pCpu, pGpu);

	CopyBuffer(stagingBuffer, m_Buffer, bufferSize, pCommandPool->GetPool(), pCpu);

	vkDestroyBuffer(pCpu->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pCpu->GetDevice(), stagingBufferMemory, nullptr);
}

IndexBuffer::~IndexBuffer()
{
	vkDestroyBuffer(m_pCpu->GetDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_pCpu->GetDevice(), m_BufferMemory, nullptr);
}