#include "CommandPool.h"

#include "LogicalDevice.h"
#include "PhysicalDevice.h"

CommandPool::CommandPool(LogicalDevice* pCpu, PhysicalDevice* pGpu):
	m_pCpu(pCpu)
{
	const QueueFamilyIndices queueFamilyIndices = pGpu->GetDesc().QueueIndices;

	//Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we retrieved.
	//Each command pool can only allocate command buffers that are submitted on a single type of queue.
	//We're going to record command for drawing, which is why we've chosen the graphics queue family.

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;

	//there are two possible flags fro command pools:
	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: hint that command buffers are rerecorded with new command very often
	//This may change memory allocatoin behaviour)

	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individaully, without this flag they all have to be reset together.

	//We will only record the command buffers at the beginning of the program and then execute them many times
	//in the main loop, so we're not going to use either of these flags.
	poolInfo.flags = 0; //Optional

	if (vkCreateCommandPool(pCpu->GetDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

CommandPool::~CommandPool()
{
	vkDestroyCommandPool(m_pCpu->GetDevice(), m_CommandPool, nullptr);
}