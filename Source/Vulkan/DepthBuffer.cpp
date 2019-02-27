#include "DepthBuffer.h"

#include "LogicalDevice.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "Buffer2D.h"

#include "../Help/HelperMethods.h"

DepthBuffer::DepthBuffer(LogicalDevice* pCpu, CommandPool* pCommandPool, RenderPass* pRenderPass, PhysicalDevice* pGpu, uint32_t width, uint32_t height)
{
	//Creating a depth image is fairly straightforward. It should have the same resolution as the color
	//attachment, defined by the swap chain extent, an image usage appropriate for a depth attachment, 
	//optimal tiling and device local memory.

	//Unlike the texture image, we don't necessarily need a specific format, because we won't be directly accessing
	//the texels from the program. It just needs to have a reasonable accuracy, at least 23 bits is common in real-world applications.
	//There are several formats that fit this requirement:
	//VK_FORMAT_D32_SFLOAT: 32-bit float for depth
	//VK_FORMAT_D32_SFLOAT_S8_UINT: 32-bit signed float for depth and 8 bit stencil component
	//VK_FORMAT_D24_UNORM_S8_UINT: 24-bit float for depth and 8 bit stencil component
	//The stencil component is used for stencil tests, which is an additional test that can be combined with depth testing.
	
	m_Buffer = std::make_unique<Buffer2D>(pCpu, pCommandPool, pRenderPass, pGpu,
		width, height, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FindDepthFormat(pGpu), VK_IMAGE_ASPECT_PLANE_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	//VkFormat depthFormat = FindDepthFormat(m_UniqueGpu.get());
	//CreateImage(m_UniqueSwapChain->GetExtent().width, m_UniqueSwapChain->GetExtent().height, 1, m_UniqueRenderPass->GetSamplesCount(), depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory, m_UniqueCpu.get(), m_UniqueGpu.get());
	//m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, m_UniqueCpu.get());

	////The undefined layout can be used as initial layout, becasue there are no existing depth image contets that matter.
	////We need to update some of the logic in transitionImageLayout to use the right subrouse aspect.
	//TransitionImageLayout(m_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, m_UniqueCommandPool.get(), m_UniqueCpu.get());
}

DepthBuffer::~DepthBuffer()
{

}