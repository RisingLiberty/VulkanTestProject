#include "VertexBuffer.h"

#include "../Help/HelperMethods.h"

#include "LogicalDevice.h"
#include "CommandPool.h"

VertexBuffer::VertexBuffer(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandPool, const std::vector<Vertex>& vertices):
	m_pCpu(pCpu)
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	//CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertexBuffer, m_VertexBufferMemory);

	VkBuffer staginBuffer;
	VkDeviceMemory stagingBufferMemory;

	//We're now using a new staginBuffer with staginBufferMemory for mapping and copying the vertex data.
	//In this chapter we're going to use 2 new buffer flags:
	//VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
	//VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.

	//The m_VertexBuffer is now allocated from a memory type that is device local, which generally means
	//that we're not able to use vkMapMemory. However, we can copy data from the stagingBuffer to the m_VertexBuffer.
	//We have to indicate that we inted to do that by specifying the transfer source flag for the stagingBuffer and
	//the transfer destination flag for the m_VertexBuffer, along with the vrtex buffer usage flag.
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staginBuffer, stagingBufferMemory, pCpu, pGpu);

	void* data;
	vkMapMemory(pCpu->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(pCpu->GetDevice(), stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory, pCpu, pGpu);
	CopyBuffer(staginBuffer, m_Buffer, bufferSize, pCommandPool->GetPool(), pCpu);

	vkDestroyBuffer(pCpu->GetDevice(), staginBuffer, nullptr);
	vkFreeMemory(pCpu->GetDevice(), stagingBufferMemory, nullptr);

	//It should be noted that in real world applications, you're not supposed to actually call vkAllocateMemory
	//for every individual buffer. The Maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount
	//physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080.
	//The right way to allocate memory for a large number of objects at the same time is to create a custom allocator
	//that splits up a single allocatoin among many different objects by using the offset parameters that we've
	//seen in many functions.

	//You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiave.
	//However, for this tutorial it's okay to use a seperate allocation for every resource, because we won't come close
	//to hitting any of these limits for now.

	//Copy vertices into buffer
	//Unfortunately the driver may not immediately copy the data into the buffer memory.,
	//for example because of caching. It is also possible that writings to the buffer are not viisble
	//in the mapped memory yet. There are 2 ways to deal with that problem:
	//1) Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT.
	//2) Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges
	//before reading from the mapped memory.

	//We went for the first approach, which ensures that the mapped memory always matches the contents of the allocated memory.
	//Do keep in mind that this may lead to slightly worse performance than explicit flushing.
	//but we'll see why that doesn't matter in the next chapter (Vulkan Tutorial page 168)
	//void* data;
	//vkMapMemory(m_UniqueCpu->GetDevice(), m_VertexBufferMemory, 0, bufferSize, 0, &data);
	//memcpy(data, vertices.data(), (size_t)bufferSize);
	//vkUnmapMemory(m_UniqueCpu->GetDevice(), m_VertexBufferMemory);
}

VertexBuffer::~VertexBuffer()
{
	vkDestroyBuffer(m_pCpu->GetDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_pCpu->GetDevice(), m_BufferMemory, nullptr);

}