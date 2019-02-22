#include "LogicalDevice.h"

#include <vector>
#include <set>

#include "PhysicalDevice.h"
#include "VulkanInstance.h"

LogicalDevice::LogicalDevice(VulkanInstance* pInstance, PhysicalDevice* pGpu, const std::vector<const char*>& extensions, const std::vector<const char*>& validationLayers)
{
	//The creatoin of a logical device involves specifying a bunch of details in structs again.
	//The first one will be VkDeviceQueueCreateInfo. this structure describes the number of queues we want
	//for a single queue family. Right now we're only interested in a queue with graphic capabilities.
	const QueueFamilyIndices indices = pGpu->GetDesc().QueueIndices;

	//The currently available drives will only allow you to create a small number of queues for each queue family
	//and you don't really need more than one. That's because you can create all of the command buffers on multiple
	//threads and then submit them all at once on the main thread with a single low-overhead call.

	//Vulkan lets you assign priorities to queues to influence the scheduling of command buffer exectuion using
	//floating point numbers between 0.0 and 1.0. This is required even if there is only a single queue.
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

	float queuePriority = 1.0f;

	for (int queueFamiliy : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.GraphicsFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		//check if this should change to push_back
		queueCreateInfos.emplace_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//First add pointers to the queue creatoin info and device features structs
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	//The next informatoin to specify is the set of device features that we'll be using.
	//These are the features that we queried support for with vkGetPhysicalDeviceFeatures previously, 
	//like geometry shader.
	VkPhysicalDeviceFeatures& deviceFeatures = pGpu->GetDescRef().Features;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; //enable sample shading feature for the device

	//The remainder of the information bears a resemblance to the VkInstanceCreateInfo struct and requires
	//you to specify extensions and validation layers. The difference is that these are device specific this time.
	createInfo.pEnabledFeatures = &deviceFeatures;

	//An example of a device specific extension is VK_KHR_swapchain, which allows you to present rendered
	//images from that device to windows. It is possible that there are Vulkan devices in the system that lack
	//this ability, for example because they only support compute operations. 
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	//we will enable the same validatoin layers for devices as we did for the instance.
	//We won't need any device specific extensoins for now.
	if (pInstance->AreValidationLayersEnabled())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(pGpu->GetDevice(), &createInfo, nullptr, &m_Device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_PresentQueue);
}

LogicalDevice::~LogicalDevice()
{
	vkDestroyDevice(m_Device, nullptr);
}
