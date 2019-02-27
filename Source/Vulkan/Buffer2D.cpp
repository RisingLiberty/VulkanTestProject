#include "Buffer2D.h"

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "SwapChain.h"

#include "../Help/HelperMethods.h"

Buffer2D::Buffer2D(LogicalDevice* pCpu, CommandPool* pCommandPool, RenderPass* pRenderPass, PhysicalDevice* pGpu, 
	uint32_t width, uint32_t height, VkImageTiling tiling, VkImageUsageFlags usage, 
	VkMemoryPropertyFlags flags, VkFormat format, VkImageAspectFlagBits aspectFlagBits, 
	VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) :
	m_pCpu(pCpu)
{
	CreateImage(width, height, mipLevels,
		pRenderPass->GetSamplesCount(), format,
		tiling, usage, flags,
		m_Image, m_ImageMemory,
		pCpu, pGpu);

	m_ImageView = CreateImageView(m_Image, format, aspectFlagBits, mipLevels, pCpu);

	TransitionImageLayout(m_Image, format, oldLayout, newLayout, mipLevels, pCommandPool, pCpu);

}

Buffer2D::~Buffer2D()
{
	vkDestroyImageView(m_pCpu->GetDevice(), m_ImageView, nullptr);
	vkDestroyImage(m_pCpu->GetDevice(), m_Image, nullptr);
	vkFreeMemory(m_pCpu->GetDevice(), m_ImageMemory, nullptr);

}