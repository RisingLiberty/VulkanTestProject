#include "RenderPass.h"

#include <array>

#include "SwapChain.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

#include "../Help/HelperMethods.h"

RenderPass::RenderPass(LogicalDevice* pDevice, SwapChain* pSwapchain, PhysicalDevice* pGpu):
	m_pDevice(pDevice),
	m_msaaSamples(pGpu->GetMaxUsableSampleCount())
{
	VkAttachmentDescription colorAttachment = {};

	//the format of the color attachment should match the format of the swap chain images.
	//and we're not doing anything with multisampling yet, so we'll stick to 1 sample.
	colorAttachment.format = pSwapchain->GetFormat();
	colorAttachment.samples = m_msaaSamples;

	//The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering.
	//we have the following choices for loadOp:
	//VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachement
	//VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
	//VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them.

	//We're going to clear the framebuffer to black before drawing a new frame so we use the clear option
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	//There are only 2 possiblities for the storeOp:
	//VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later.
	//VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after rendering operation.

	//We're interested in seeing the rendered triangle on the screen, so we're going with the store operation here.
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//The loadOp and storeOp apply to color and depth data, and stencilLoadOp / stencilStoreOp apply to stencil data.
	//Our applocation won't do anything with the stencil buffer, so the results of loading and storing are irrelevant.
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Textures and framebuffers in Vulkan are represented by VkImage objects with a certai pixel format,
	//however the layout of the pixels in memory can change based on what you're trying to do with an image.

	//Some of the most common layouts are:
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
	//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: images to be presented in the swap chain.
	//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: images to be used as destination for a memory copy operation

	//Others:
	//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: Optimal as source in a transfer operation
	//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: Optimal for sampling from a shader

	//The initialLayout specifies which layout the image will have before the render pass begins.
	//The finalLayout specifies the layout to automatically transition to when the render pass finishes.
	//Using VK_IMAGE_LAYOUT_UNDEFINED for initalLayout means that we don't care what previous layout the image was in.
	//The caveat of this special value is that the contents of the image are not guaranteed to be preserved, but that doesn't
	//matter since we're going to clear it anywawy. We want the image to be ready for presentation using the swap chain
	//after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.
	//MSAA: finalLayout --> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	//this change is because multisampled images cannot be presented directly.
	//We first need to resolve them to a regular image. this requirement doest not apply to the depth buffer,
	//since we won't be presented at any point. Therefore we will have to add onl one new attachment for color
	//which is a so-called resolve attachment.
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = pSwapchain->GetFormat();
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};

	//The format should be the same as the depth image itself.
	depthAttachment.format = FindDepthFormat(pGpu);
	depthAttachment.samples = m_msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	//This time we don't care about storing the depth data, because it will not be used after drawing has finished.
	//This may allow the hardware to perfrom additional optimizatoins.
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Just like the color buffer, we don't care about the previous depth contents, so we can use VK_IMAGE_LAYOUT_UNDEFINED as initialLayout.
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	//the attachment parameter specifies which attachment to reference by its index in the attachment descriptions array.
	//Out array consists of a single vkAttachmentDescription, so its index is 0.
	//The layout specifies which layout we would like the attachment to have during a subpass that uses this reference.
	//Vulkan will automatically transition the attachment to this laoyut when the subpass is started.
	//We inted to use the attachment to function as a color buffer and the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout
	//will give us the best performance, as its name implies

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//MSAA: the render pass has to be instructed to resolve multisampled color image into regular attachment
	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	//the index of the attachment in this array is directly referenced from the fragment shader
	//with the layout(location = 0) out vec4 outColor directive!
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	//Add a reference to the attachment for the first (and only) subpass
	//Unlike color attachments, a subpass can only use a single depth(+ stencil) attachment.
	//It wouldn't really make any sense to do depth test on multiple buffers.
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	//pInputAttachments: attachments that are read from a shader
	//pResolveAttachments: attachments used for multisampling color attachments
	//pDepthStencilAttachment: attachments for depth and stencil data
	//pPreserveAttachments: attachments that are not used by this subpass, but for which the data must be preserved.
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	//the first 2 fields specify the indices of the dependency and the dependent subpass. The special value VK_SUBPASS_EXTERNAL refers to
	//the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass.
	//The index 0 refers to our subpass, which is the first and only one.
	//The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph.
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	//The next two fields specify the operations to wait on and the stages in which these operation occur.
	//We need to wait for the swap chain to finish reading from the image before we can access it.
	//This can be accomplished by waiting on the color attachment output stage itself.
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//The operation that should wait on this are the color attachment stage and involve the reading and writing of the color attachment.
	//These settings will prevent the transition from happening until it's actually necessary (and allowed).
	//this is when we want to start writing colors to it.
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_pDevice->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass");
}

RenderPass::~RenderPass()
{
	vkDestroyRenderPass(m_pDevice->GetDevice(), m_RenderPass, nullptr);
}