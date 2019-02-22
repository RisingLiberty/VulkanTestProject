#include "PhysicalDevice.h"

#include "Surface.h"

PhysicalDevice::PhysicalDevice(Surface* pSurface, const VkPhysicalDevice& device, std::set<std::string>& requiredExtensions) :
	m_Device(device),
	m_pSurface(pSurface),
	m_RequiredExtensions(requiredExtensions)
{
	this->Initialize();
}


PhysicalDevice::~PhysicalDevice()
{
	
}

bool PhysicalDevice::IsSuitable() const
{
	bool extensionsSupported = CheckDeviceExtensionSupport(m_RequiredExtensions);
	bool swapChainAdequate = !m_Desc.SwapChainSupportDetails.Formats.empty() && !m_Desc.SwapChainSupportDetails.PresentModes.empty();
	
	return m_Desc.QueueIndices.IsComplete() && extensionsSupported && swapChainAdequate && m_Desc.Features.samplerAnisotropy;
}

bool PhysicalDevice::CheckDeviceExtensionSupport(std::set<std::string>& requiredExtensions) const
{
	for (const VkExtensionProperties& props : m_Desc.AvailableExtensions)
		requiredExtensions.erase(props.extensionName);

	return requiredExtensions.empty();
}


int PhysicalDevice::RateSuitability() const
{
	int score = 0;

	//Application can't function without geometry shaders
	if (!m_Desc.Features.geometryShader)
		return score;
	
	//Discrete gpus have a significant performance advantage
	if (m_Desc.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	//Maximum possible size of texture affect graphics quality
	score += m_Desc.Properties.limits.maxImageDimension2D;

	return score;
}

void PhysicalDevice::Initialize()
{
	m_Desc = {};

	//Properties
	//-------------------------
	vkGetPhysicalDeviceProperties(m_Device, &m_Desc.Properties);

	//Extensions
	//------------------------
	m_Desc.AvailableExtensions = FindExtentions();

	//Swapchain Support Details
	//-------------------------
	bool extensionsSupported = CheckDeviceExtensionSupport(m_RequiredExtensions);

	if (extensionsSupported)
		m_Desc.SwapChainSupportDetails = FindSwapChainSupport();

	//Features
	//-------------------------
	vkGetPhysicalDeviceFeatures(m_Device, &m_Desc.Features);

	//QueueFamilies
	//-------------------------
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_Device, &queueFamilyCount, nullptr);

	//The VkQueueFamilyProperties struct contains some details about the queue family, 
	//including the type of operations that are supported and the number of queues that can be created based on that family.
	//We need to find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
	m_Desc.QueueFamilies.resize(queueFamilyCount);

	//TODO: Reform this so we can store immediatly and shrink the vector to size
	vkGetPhysicalDeviceQueueFamilyProperties(m_Device, &queueFamilyCount, m_Desc.QueueFamilies.data());

	//Queue Family Indices
	//------------------------
	m_Desc.QueueIndices = FindQueueFamilies(m_pSurface);

	//Memory Properties
	//------------------------
	vkGetPhysicalDeviceMemoryProperties(m_Device, &m_Desc.MemProperties);
	
}

//Anything from drawing to uploading textures, requires commands to be submitted to a queue.
//There are different types of queues that orginate from different queue families and each family of queues
//allows only a subset of commands.
//For example, there could be a queue family that only allows processing of compute commands or one that only
//allows memory transfer related commands.


//This function will return the indices of the queue families that satisfy certain desired properties.
//The best way to do that is using a structure, where an index of -1 will denote "not found".
QueueFamilyIndices PhysicalDevice::FindQueueFamilies(Surface* pSurface) const
{
	QueueFamilyIndices indices;

	//Get nr of queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_Device, &queueFamilyCount, nullptr);

	int i = 0;
	for (const VkQueueFamilyProperties queueFamily : m_Desc.QueueFamilies)
	{
		//We need to check which queue families are supported by the device and which one of these supports the
		//commands that we want to use. For that purpose we have this function that looks for all the queue families.
		//we need. Right now we'll only look for a queue that support the graphics commands.
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.GraphicsFamily = i;

		if (indices.IsComplete())
			break;

		//Check for a queue family that has the capability of presenting to our window surface.
		//The function to check for that is vkGetPhysicalDeviceSurfaceSupportKHR, which takes the
		//physical device, queue family index and surface as parameters.
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_Device, i, pSurface->GetSurface(), &presentSupport);
		if (queueFamily.queueCount >= 0 && presentSupport)
			indices.PresentFamily = i;

		++i;
	}

	return indices;

}

std::vector<VkExtensionProperties> PhysicalDevice::FindExtentions() const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions;

	extensions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, extensions.data());

	return extensions;
}

SwapChainSupportDetails PhysicalDevice::FindSwapChainSupport() const
{
	//This function takes the specified VkPhysicalDEvice and VkSurfaceKHR window surface into account
	//when determining the supported capabilities. All of the support querying functions have these two
	//as first parameters because they are the core components of the swap chain.
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device, m_pSurface->GetSurface(), &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_pSurface->GetSurface(), &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_pSurface->GetSurface(), &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_pSurface->GetSurface(), &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_pSurface->GetSurface(), &presentModeCount, details.PresentModes.data());
	}

	return details;
}