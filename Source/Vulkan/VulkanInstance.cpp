#include "VulkanInstance.h"

#include <iostream>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	//Message is important enough to show
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		std::cerr << "fatal error" << std::endl;

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}


VulkanInstance::VulkanInstance(bool enableValidationLayers):
	m_EnableValidationLayers(enableValidationLayers)
{
	//If we have validation layers enabled, we need to check if we actually support them
	if (m_EnableValidationLayers && !CheckValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available");
	
	VkInstanceCreateInfo createInfo1 = MakeInstanceCreateInfo();
	if (vkCreateInstance(&createInfo1, nullptr, &m_Instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance");

	SetupDebugCallback();
}

VulkanInstance::~VulkanInstance()
{
	if (m_EnableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_Callback);

	vkDestroyInstance(m_Instance, nullptr);
}

bool VulkanInstance::CheckValidationLayerSupport() const
{
	uint32_t layerCount;
	//it takes a pointer to a variable that stores the number of layers and an array of
	//VkLayerProperties to store the details of the layers.
	//First get the count to initialize the vector size
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	//now we can get the details
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	std::cout << "available layers: " << std::endl;

	for (const char* layerName : m_ValidationLayers)
	{
		bool layerFound = false;

		for (const VkLayerProperties& layerProps : availableLayers)
		{
			std::cout << "\t" << layerProps.layerName << std::endl;

			if (strcmp(layerName, layerProps.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;

}

void VulkanInstance::ShowExtensions() const
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "available extensions: " << std::endl;

	for (const VkExtensionProperties extension : extensions)
		std::cout << "\t" << extension.extensionName << std::endl;
}


void VulkanInstance::InitRequiredExtentions()
{
	ShowExtensions();

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (m_EnableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	m_RequiredExtentions = std::move(extensions);
}

VkApplicationInfo VulkanInstance::MakeAppInfo() const
{
	//First we need to fill a struct with some information about our application.
	//This data is technically optional, but it may provice some useful information to the driver to
	//optimize for our specific application.
	VkApplicationInfo appInfo = {};

	//always specify this field!
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Vulkan!";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//points to extension information in the future
	appInfo.pNext = nullptr;

	return appInfo;
}

VkInstanceCreateInfo VulkanInstance::MakeInstanceCreateInfo()
{
	InitRequiredExtentions();

	VkInstanceCreateInfo createInfo = {};

	//The first 2 parameters are straightforward.
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &MakeAppInfo();

	//The next 2 members of the struct determine the gloabl validatoin layers to enable.
	if (m_EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	//The last 2 layers specify the desired global extensions.
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_RequiredExtentions.size());
	createInfo.ppEnabledExtensionNames = m_RequiredExtentions.data();

	return createInfo;
}

void VulkanInstance::SetupDebugCallback()
{
	if (!m_EnableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; /* |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; */

	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr; //Optional

	if (CreateDebugUtilsMessengerEXT(&createInfo, &m_Callback) != VK_SUCCESS)
		throw std::runtime_error("failed to set up debug callback!");

}

VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
		return func(m_Instance, pCreateInfo, nullptr, pCallback);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT callback)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(m_Instance, callback, nullptr);
}
