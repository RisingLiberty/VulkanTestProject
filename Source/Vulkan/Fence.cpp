#include "Fence.h"

#include "LogicalDevice.h"

Fence::Fence(LogicalDevice* pCpu) :
	m_pCpu(pCpu)
{

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	//Initialize a fence in the signaled state as if we already rendered a frame
	//this is to make sure the VkWaitForFences doesn't wait for ever for the first frame to be rendered.
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(pCpu->GetDevice(), &fenceInfo, nullptr, &m_Fence) != VK_SUCCESS)
		throw std::runtime_error("failed to create fence!");
}

Fence::~Fence()
{
	vkDestroyFence(m_pCpu->GetDevice(), m_Fence, nullptr);
}