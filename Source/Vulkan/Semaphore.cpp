#include "Semaphore.h"

#include <stdexcept>

#include "LogicalDevice.h"

Semaphore::Semaphore(LogicalDevice* pCpu):
	m_pCpu(pCpu)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(pCpu->GetDevice(), &semaphoreInfo, nullptr, &m_Semaphore) != VK_SUCCESS)
		throw std::runtime_error("Failed to create semaphore");
}

Semaphore::~Semaphore()
{
	vkDestroySemaphore(m_pCpu->GetDevice(), m_Semaphore, nullptr);
}