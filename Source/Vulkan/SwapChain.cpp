#include "SwapChain.h"

#include "PhysicalDevice.h"
#include "../Core/Window.h"
#include "Surface.h"
#include "LogicalDevice.h"

#include "../Help/HelperMethods.h"

#include <array>
#include <chrono>



SwapChain::SwapChain(PhysicalDevice* pPhysicalDevice, Window* pWindow, Surface* pSurface, LogicalDevice* pCpu):
	m_pWindow(pWindow),
	m_pCpu(pCpu),
	m_pPhysicalDevice(pPhysicalDevice)
{
	const SwapChainSupportDetails swapChainSupport = pPhysicalDevice->GetDesc().SwapChainSupportDetails;

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapChainPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.minImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
		imageCount = swapChainSupport.Capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = pSurface->GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const QueueFamilyIndices indices = pPhysicalDevice->GetDesc().QueueIndices;
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.GraphicsFamily, (uint32_t)indices.PresentFamily };

	if (indices.GraphicsFamily != indices.PresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; //Optional
		createInfo.pQueueFamilyIndices = nullptr; //Optional
	}

	//We can specify that a certain transform should be applied to images in the swap chain if 
		//it is supported (supportedTransforms in capabilities), like a 90 degree clockwise rotation
		//or horizontal flip. to specify that you do not want any transformation, simply specify the current transformation.
	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;

	//The compositeAplha field specifies if the alpha channel should be used for blending with other windows in the window system
	//You'll almost always want to simly ignore the alpha channel hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	//The presentmode member speaks for itself. if the clipped member is set to VK_TRUE then that means that we don't
	//care about the color of pixels that are obscured, for example because another window is in front of them.
	//Unless you really need to be able to read these pixels back and get predictable results, you'll get the best 
	//performance by enabling clipping.
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//That leaves one last field, oldSwapChain. with Vulakn it's possible that your swap chain becomes invalid or unoptimized,
	//while your application is running, for example because the window was resized. In that case the swap chain actually,
	//needs to be recreated from scratch and a reference to the old one must be specified in this field.
	//This is a complex topic that we'll learn more about in a future chapter. For now we'll assume that we'll only ever create one swap chain.
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(pCpu->GetDevice(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain!");

	vkGetSwapchainImagesKHR(pCpu->GetDevice(), m_SwapChain, &imageCount, nullptr);
	m_Images.resize(imageCount);
	vkGetSwapchainImagesKHR(pCpu->GetDevice(), m_SwapChain, &imageCount, m_Images.data());

	m_SwapChainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;
}

SwapChain::~SwapChain()
{
	for (size_t i = 0; i < m_FrameBuffers.size(); ++i)
		vkDestroyFramebuffer(m_pCpu->GetDevice(), m_FrameBuffers[i], nullptr);

	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		vkDestroyBuffer(m_pCpu->GetDevice(), m_UniformBuffers[i], nullptr);
		vkFreeMemory(m_pCpu->GetDevice(), m_UniformBuffersMemory[i], nullptr);
	}

	for (size_t i = 0; i < m_ImageViews.size(); ++i)
		vkDestroyImageView(m_pCpu->GetDevice(), m_ImageViews[i], nullptr);

	vkDestroySwapchainKHR(m_pCpu->GetDevice(), m_SwapChain, nullptr);

}

VkSurfaceFormatKHR SwapChain::ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

void SwapChain::CreateImageViews()
{
	m_ImageViews.resize(m_Images.size());

	for (size_t i = 0; i < m_Images.size(); ++i)
	{
		m_ImageViews[i] = CreateImageView(m_Images[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_pCpu);
	}
}

void SwapChain::CreateFrameBuffers(const VkRenderPass& renderPass, const VkImageView& colorImageView, const VkImageView& depthImageView)
{
	m_FrameBuffers.resize(m_ImageViews.size());

	for (size_t i = 0; i < m_ImageViews.size(); ++i)
	{
		std::array<VkImageView, 3> attachements =
		{
			colorImageView,
			depthImageView,
			m_ImageViews[i]
		};

		//as you can see, creation of framebuffers is quite straightforward.
		//We first need to specify with which renderPass the framebuffer needs to be compatible.
		//You can only use a framebuffer with the render passes that it is compatible with,
		//which roughly means that they use the same number and type of attachments.
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;

		//the attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound
		//to the respective attachment descriptions in the render pass pAttachment array.
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachements.size());
		framebufferInfo.pAttachments = attachements.data();

		//The width and height parameters are self-explanatory and layers reders to the number of layers in image arrays
		//Our swap chain images are single images, so the number of layers is 1.
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_pCpu->GetDevice(), &framebufferInfo, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}

}

void SwapChain::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_UniformBuffers.resize(m_Images.size());
	m_UniformBuffersMemory.resize(m_Images.size());

	for (size_t i = 0; i < m_Images.size(); ++i)
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i], m_pCpu, m_pPhysicalDevice);

}

VkPresentModeKHR SwapChain::ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	else
	{
		int width, height;
		glfwGetFramebufferSize(m_pWindow->GetGLFWWindow(), &width, &height);

		VkExtent2D actualtExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		return actualtExtent;
	}
}

void SwapChain::UpdateUniformBuffer(uint32_t currentImage)
{
	//this function will generate a new transformation every frame to make the geometry spin around.
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo = {};
	//The glm::rotate takes in an exisiting transformation, rotation angle and rotatoin axis as parameters.
	//the glm::mat4(1.0f) constructor return an identity matrix.
	//Using a rotation of time * glm::radians(90.f) accomplishes the purpose of rotation 90 degrees per second.

	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	//For the view transformation I've decided to look a tthe geometry form above at a 45 degree angle.
	//The glm::lookAt function takes the eye position, center position and up axis as parameters.
	ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	//I've chosen to use a perspective projection with a 45 degree angle vertical fov.
	//The other parameters are the aspect ratio, near and far view planes.
	//it is important to use the current swapchain extent to calculate the aspect ration to take into account the
	//new width and height of the window after a resize.
	ubo.Proj = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.f);

	//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
	//The easies way to compensate for that is to flip the sign on the scaling factor of the Y axis in the proj matrix.
	//If you don't do this, then the image will be rendered upside down.
	ubo.Proj[1][1] *= -1;

	//Using a UBO this way is not the moest efficient way to pass frequently changing values to the shader.
	//A more fficine tway to pass a smaal buffer of data to shaders push constants.
	//https://stackoverflow.com/questions/50956414/what-is-a-push-constant-in-vulkan
	//https://github.com/PacktPublishing/Vulkan-Cookbook
	//https://github.com/SaschaWillems/Vulkan
	void* data;
	vkMapMemory(m_pCpu->GetDevice(), m_UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_pCpu->GetDevice(), m_UniformBuffersMemory[currentImage]);

}
