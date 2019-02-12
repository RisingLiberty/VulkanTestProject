#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>

//Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
//Fixed-function state: all of the structures that define the fixed-functoins stages of the pipeline, like input assembly, rasterizer, viewport and color blending
//Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
//Render pass: the attachments referenced by the pipeline stages and their usage

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
	int GraphicsFamiliy = -1;
	int PresentFamily = -1;

	bool IsComplete() { return GraphicsFamiliy >= 0 && PresentFamily >= 0; }
};

static std::vector<char> ReadFile(const std::string& fileName)
{
	//start reading at the end of the file
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	//because our read position is at the end, we can determine the size of the file and allocate a buffer
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	//set the read position back to the first char.
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	
	return buffer;
}

class HelloTriangleApplication
{
public:
	void Run()
	{
		InitializeWindow();
		InitializeVulkan();
		MainLoop();
		Cleanup();
	}

private:
	void InitializeWindow()
	{
		glfwInit();

		//Hint that we're not making the window for opengl
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		//hint that the window can't be resized
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void InitializeVulkan()
	{
		CreateInstance();
		DisplayExtentions();
		SetupDebugCallback();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSemaphores();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
			DrawFrame();
		}
	}

	void Cleanup()
	{
		//Clean vulkan
		if (m_EnableValidationLayers)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_Callback, nullptr);

		vkDestroySemaphore(m_Device, m_RenderFinishedSamephore, nullptr);
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);

		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		for (VkFramebuffer framebuffer : m_SwapChainFrameBuffers)
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (VkImageView imageView : m_SwapChainImageViews)
			vkDestroyImageView(m_Device, imageView, nullptr);

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);


		vkDestroyInstance(m_Instance, nullptr);

		//Clean glfw
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}

	void CreateInstance()
	{
		if (m_EnableValidationLayers && !CheckValidationLayerSupport())
			throw std::runtime_error("validation layers requested, but not available");

		VkApplicationInfo appInfo = {};

		//sType needs to be set
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle!";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		std::vector<const char*> extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (m_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}
		else
			createInfo.enabledLayerCount = 0;

		//uint32_t glfwExtensionCount = 0;
		//const char** glfwExtentions;

		//glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		////global desired extensions
		//createInfo.enabledExtensionCount = glfwExtensionCount;
		//createInfo.ppEnabledExtensionNames = glfwExtentions;

		//global validation layers
		//createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
			throw std::runtime_error("failed to create instance");
	}

	void DisplayExtentions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "available extensions: " << std::endl;

		for (const VkExtensionProperties extension : extensions)
			std::cout << "\t" << extension.extensionName << std::endl;
	}

	bool CheckValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_ValidationLayers)
		{
			bool layerFound = false;

			for (const VkLayerProperties& layerProps : availableLayers)
			{
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

	std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_EnableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		//Message is important enough to show
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			std::cerr << "fatal error" << std::endl;

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void SetupDebugCallback()
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

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_Callback) != VK_SUCCESS)
			throw std::runtime_error("failed to set up debug callback!");

	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pCallback);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, callback, pAllocator);
	}

	void PickPhysicalDevice()
	{
		uint32_t gpuCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);

		if (gpuCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> GPUs(gpuCount);
		vkEnumeratePhysicalDevices(m_Instance, &gpuCount, GPUs.data());

		for (const VkPhysicalDevice& gpu : GPUs)
		{
			if (IsGpuSuitable(gpu))
			{
				m_PhysicalDevice = gpu;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("failed to find a suitbale GPU!");

		//Use an ordered map to automatically sort caindiates by increasing score
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const VkPhysicalDevice& device : GPUs)
		{
			int score = RateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		//Check if th best candidate is suitable at all
		if (candidates.rbegin()->first > 0)
			m_PhysicalDevice = candidates.rbegin()->second;
		else
			throw std::runtime_error("failed to find a suitable GPU!");

		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProps);
		std::cout << "Using: " << deviceProps.deviceName << std::endl;
	}

	bool IsGpuSuitable(VkPhysicalDevice device)
	{
		//VkPhysicalDeviceProperties deviceProperties;
		//vkGetPhysicalDeviceProperties(device, &deviceProperties);
		//
		//VkPhysicalDeviceFeatures deviceFeatures;
		//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		//
		//GPU needs to support geometry shader
		//return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;

		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.IsComplete() && extensionsSupported && swapChainAdequate;
	}

	int RateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		//device supports geometry shaders
		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;

		int score = 0;

		//Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		//Maximum possible size of textures affect graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		//Application can't function without geometry shaders
		if (!deviceFeatures.geometryShader)
			return 0;

		return score;
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamiliyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.GraphicsFamiliy = i;

			if (indices.IsComplete())
				break;


			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
			if (queueFamily.queueCount >= 0 && presentSupport)
				indices.PresentFamily = i;

			++i;
		}


		return indices;
	}

	void CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.GraphicsFamiliy, indices.PresentFamily };

		float queuePriority = 1.0f;

		for (int queueFamiliy : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = indices.GraphicsFamiliy;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			//check if this should change to push_back
			queueCreateInfos.emplace_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());


		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		if (m_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
			createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		}
		else
			createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
			throw std::runtime_error("failed to create logical device!");

		vkGetDeviceQueue(m_Device, indices.GraphicsFamiliy, 0, &m_GraphicsQueue);
	}

	void CreateSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_Surface) != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface!");
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionsCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const VkExtensionProperties& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR ChooseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

	VkPresentModeKHR ChooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
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

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			VkExtent2D actualtExtent = { WIDTH, HEIGHT };

			actualtExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualtExtent.width));
			actualtExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualtExtent.height));

			return actualtExtent;
		}
	}

	void CreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapChainSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapChainPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (swapChainSupport.Capabilities.minImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
			imageCount = swapChainSupport.Capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.GraphicsFamiliy, (uint32_t)indices.PresentFamily };

		if (indices.GraphicsFamiliy != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; //optional
			createInfo.pQueueFamilyIndices = nullptr; //optional
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

		if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain!");

		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		m_SwapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent = extent;
	}

	void CreateImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());

		for (size_t i = 0; i < m_SwapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo = {};

			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapChainImages[i];

			//The viewType and format fields specify how the image data should be interpreted.
			//the viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cubemaps
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SwapChainImageFormat;

			//The components field allows you to swizzle the color channels around. 
			//for example, you can map all of the channels to the red channel for a monochrome texture.
			//you can also map constant values of 0 and 1 to a channel. In our case we'll stick to the default mapping.
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			
			//the subresourceRange field describes what the image's purpose is and which part of the image should be accessed.
			//Our images will be used as color targets without any mipmapping levels or multiple layers.
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create image views!");
		}
	}

	void CreateGraphicsPipeline()
	{
		std::vector<char> vertShaderCode = ReadFile("../data/shaders/bin/vert.spv");
		std::vector<char> fragShaderCode = ReadFile("../data/shaders/bin/frag.spv");

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		//the next 2 members specify the shader module containing the code, and the function to invoke.
		//that means that it's possible to combine multiple fragment shaders into a single shader module and
		//and use different entry points to differentiate between their behaviors. In this case we'll stick to the standard main.

		//However there is one more (optional) member, pSpecializationInfo, which we won't be using here, but is worth discussing.
		//It allows you to specify values for shader constants. You can use a single shader module where its behavior
		//can be used in it. This is more efficient than configuring the shader using variables at render time,
		//because the compiler can do optimizations like eliminating if statemetns that depend on these values.
		//If you don't have any constans like that, then you can set the member to nullptr, which our struct initialization does automatically.
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		//The pVertexBindingDescriptions and pVertexAttributeDescriptions members point to an array
		//of structs that describe that aformentinoed details for loading vertex data.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; //Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; //Optional

		//Normally, the vertices are loaded from the vertex buffer by index in sequential order,
		//but with an element buffer you can specify the indices to use yourself.
		//This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to VK_TRUE,
		//then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapChainExtent.width;
		viewport.height = (float)m_SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0,0 };
		scissor.extent = m_SwapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns
		//it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test,
		//and it can be configured to output fragments that fill entire polygons or just the edges(wireframe rendering).
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		//if depthClampEnable is set to VK_TRUE, the fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
		//this is useful in some special cases like shadow maps. using this requires enabling a GPU feature.
		rasterizer.depthClampEnable = VK_FALSE;

		//if rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
		//this basically disables any output to the framebuffer.
		rasterizer.rasterizerDiscardEnable = VK_FALSE;

		//the polygonMode determines how fragments are generated for geometry. the following modes are available:
		//VK_POLYGON_MODE_FILL: fill the area of the polygon fragments
		//VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
		//VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

		//the linewidth member is straightforward, it describes the thickness of lines in terms of number of fragments.
		//the maximum line widht that is supported depends on the hardware and any line thicker than 1.0f requires you to enable
		//the wideLines GPU feature
		rasterizer.lineWidth = 1.0f;

		//the cullMode variable determines the type of face culling to use.
		//you can disbale culling, cull the front faces, cull the back faces or both.
		//The frontFace variable specifies the vertex order for faces to be considered front-facing and can be clockwise or counterclockwise.
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

		//the rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's lsope.
		//this is sometimes used for shadow mapping, but we won't be using it. just set depthBiasEnable to VK_FALSE
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; //Optional
		rasterizer.depthBiasClamp = 0.0f; //Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; //Optional

		VkPipelineMultisampleStateCreateInfo multiSampling = {};
		multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multiSampling.sampleShadingEnable = VK_FALSE;
		multiSampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multiSampling.minSampleShading = 1.0f; //Optional
		multiSampling.pSampleMask = nullptr; //Optional
		multiSampling.alphaToCoverageEnable = VK_FALSE; //Optional
		multiSampling.alphaToOneEnable = VK_FALSE; //Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachement = {};
		//There are two types of structs to configure color blending.
		//The first struct, VkPipelineColorBlendAttachementState contains the configuration per attached framebuffer.
		//The second struct, VkPipelineColorBlendStateCreateInfo contains the global color blending settings.
		colorBlendAttachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;;
		colorBlendAttachement.blendEnable = VK_FALSE;
		colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
		colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD; //Optional
		colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
		colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
		colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD; //Optional

		//This per-framebuffer struct allows you to configure the first way of color blending.
		//the operations that will be performed are best demonstrated using the following pseudocode

		/*
		if (blendEnable)
		{
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
		}

		else
			finalColor = newColor;
		*/
	
		//if blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified.
		//Otherwise, the two mixing operations are performed to compute a new color.
		//The resulting color AND'd with the colorWriteMask to determine which channels are actually passed through

		//The most common way to use color blending is to implement alpha blending, where we want the new color to be blended
		//with the old color based on its opacity. the finalColor should then be computed as follows.

		/*
		finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor; 
		finalColor.a = newAlpha.a;
		*/

		//this can be accomplished with the following parameters
		/*
		colorBlendAttachement.blendEnable = VK_TRUE;
		colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD;
		*/

		//the second structure references the array of structures for all of the framebuffers and allows you to 
		//set blend constants that you can use as blend factors in the aformentioned calculations.

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; //Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachement;
		colorBlending.blendConstants[0] = 0.0f; //Optional
		colorBlending.blendConstants[1] = 0.0f; //Optional
		colorBlending.blendConstants[2] = 0.0f; //Optional
		colorBlending.blendConstants[3] = 0.0f; //Optional

		//If you want to use the second method of blending(bitwise combination), then
		//you should set logicOpEnable to VK_TRUE. the bitwise operation can then be specified in the logicOp field.
		//Note that this will automatically disable the first method, as if you had set blendenable to VK_FALSE for every
		//attached framebuffer! the colorWriteMask will also be used in this mode to determine which channels in the framebuffer
		//will actually be affected. it is also possible to disable both modes, as we've done here, in which case the fragment colors
		//will be written to the framebuffer unmodified.

		//A limited amount of the state that we've specified in the previous structs can actually be changed without
		//recreating the pipeline. Examples are the size of the viewport, line width and blend constants.
		//Examples are the size of the viewport, line width and blend constant. if you want to do that, then yo'll
		//have to fill in a VkPipelineDynamicStateCreateInfo structure like this:
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; //Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; //Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; //Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; //Optional

		if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create pipeline layout");

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		//Start by referencing the array of vkPipelineShaderStageCreateInfo structs
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		//Reference all of the strucutures describing the fixed function stage
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multiSampling;
		pipelineInfo.pDepthStencilState = nullptr; //Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; //Optional

		//then the pipeline layout, which is a Vulkan handle rather than a struct pointer
		pipelineInfo.layout = m_PipelineLayout;

		//finally the renderpass and the index of the sub pass where this graphics pipeline will be used
		//It is also possible to use other render passes with this pipeline instead of this specific instance,
		//but they have to be compatible with renderPass.
		//The requirements for caompatibilty are described here <https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility>, but we wo'nt be using that feauture
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		//there are actually 2 more parameters, basePipelineHandle and basePielineIndex.
		//Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline.
		//The idea of pipeline derivatives is that is is less expesnsive to set up pipelines when they have much
		//functionality in common with an existing pipeline and switching between pipelines from the same parent
		//pipleline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
		//right now there is only a single pipeline, so we'll simply specify a null handle and an invalid index.
		//These value are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags fields of vkGraphicsPipelinecreateInfo
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //Optional
		pipelineInfo.basePipelineIndex = -1; //Optional

		//The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation functions in Vulkan.
		//It is designed to take multiple vkGraphicsPipelineCreateInfo objects and create multiple VkPipeline objects in a single call.
		//The second parameter for which we've passed VK_NULL_HANDLE argument, references an optional VkPipelineCache object.
		//A pipeline cahce can be used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphicsPipelines.
		//and even across program executions if the cache is stored to a file. This makes it possible to significantly speed up pipeline
		//creatino at a later time.
		if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create graphics pipeline!");
		
		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
	}

	VkShaderModule CreateShaderModule(const std::vector<char>& code)
	{
		//The one catch is that the sizeof the bytecode is specified in bytes, but the bytecode pointer is a
		//uint32_t pointer rather than a char pointer. Therefore we will need to cast the pointer with a reinterpret_cast
		//as shown below. When you perform a cast like this, you also need to ensure that the data satisfies the alignment
		//requirements of uint32_t. lucky for us, the data is stored in an std:vector where the default allocator already
		//ensure that the data satisfies the worst case alignment requirements.
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("failed to create shader module!");

		return shaderModule;
	}

	void CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};

		//the format of the color attachment should match the format of the swap chain images.
		//and we're not doing anything with multisampling yet, so we'll stick to 1 sample.
		colorAttachment.format = m_SwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

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

		//The initialLayout specifies which layout the image will have before the render pass begins.
		//The finalLayout specifies the layout to automatically transition to when the render pass finishes.
		//Using VK_IMAGE_LAYOUT_UNDEFINED for initalLayout means that we don't care what previous layout the image was in.
		//The caveat of this special value is that the contents of the image are not guaranteed to be preserved, but that doesn't
		//matter since we're going to clear it anywawy. We want the image to be ready for presentation using the swap chain
		//after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout.
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//the attachment parameter specifies which attachment to reference by its index in the attachment descriptions array.
		//Out array consists of a single vkAttachmentDescription, so its index is 0.
		//The layout specifies which layout we would like the attachment to have during a subpass that uses this reference.
		//Vulkan will automatically transition the attachment to this laoyut when the subpass is started.
		//We inted to use the attachment to function as a color buffer and the VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout
		//will give us the best performance, as its name implies
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		//the index of the attachment in this array is directly referenced from the fragment shader
		//with the layout(location = 0) out vec4 outColor directive!
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		//pInputAttachments: attachments that are read from a shader
		//pResolveAttachments: attachments used for multisampling color attachments
		//pDepthStencilAttachment: attachments for depth and stencil data
		//pPreserveAttachments: attachments that are not used by this subpass, but for which the data must be preserved.

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			throw std::runtime_error("failed to create render pass");
	}

	void CreateFrameBuffers()
	{
		m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

		for (size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
		{
			VkImageView attachements[] = 
			{
				m_SwapChainImageViews[i]
			};

			//as you can see, creation of framebuffers is quite straightforward.
			//We first need to specify with which renderPass the framebuffer needs to be compatible.
			//You can only use a framebuffer with the render passes that it is compatible with,
			//which roughly means that they use the same number and type of attachments.
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;

			//the attachmentCount and pAttachments parameters specify the VkImageView objects that should be bound
			//to the respective attachment descriptions in the render pass pAttachment array.
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachements;

			//The width and height parameters are self-explanatory and layers reders to the number of layers in image arrays
			//Our swap chain images are single images, so the number of layers is 1.
			framebufferInfo.width = m_SwapChainExtent.width;
			framebufferInfo.height = m_SwapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFrameBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create framebuffer!");
		}

	}

	void CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

		//Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we retrieved.
		//Each command pool can only allocate command buffers that are submitted on a single type of queue.
		//We're going to record command for drawing, which is why we've chosen the graphics queue family.

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamiliy;

		//there are two possible flags fro command pools:
		//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: hint that command buffers are rerecorded with new command very often
		//This may change memory allocatoin behaviour)

		//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individaully, without this flag they all have to be reset together.

		//We will only record the command buffers at the beginning of the program and then execute them many times
		//in the main loop, so we're not going to use either of these flags.
		poolInfo.flags = 0; //Optional

		if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create command pool!");
	}

	void CreateCommandBuffers()
	{
		m_CommandBuffers.resize(m_SwapChainFrameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;

		//The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
		//VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for exection, but cannot be called from other command buffers.
		//VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate command buffers!");

		for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			//The flags parameter specifies how we're going to use the command buffer. the following values are available:
			//VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing one.
			//VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
			//VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending exection.
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

			//This is relevant for secondary command bufffers.
			//it specifies which state to inherit from the calling primary command buffers
			beginInfo.pInheritanceInfo = nullptr; //Optional

			//If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it..'
			//it's not possiblke to append command to a buffer at a later time.

			if (vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("failed to begin recording command buffer!");

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_SwapChainFrameBuffers[i];

			//the render area defines where shader loads and stores will take place.
			//The pixels outside this region will have undefined values. It should match the size of the attachments for best performance.
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = m_SwapChainExtent;

			VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

			//These parameters define the clear value to use for VK_ATTACHMENT_LOAD_OP_CLEAR
			//which we used as load operation for the color attachment.
			//I've defined the clear color to simply be black 100% opacity
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			//VK_SUBPASS_COONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself 
			//and no secondary dommand will be executed

			//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands iwll be executed from secondary command buffers.
			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

			//The actual vkCmdDraw functoin is a bit anti climactic, but it's so simple because of all the information we specified in advance.
			//It has the following parameters, aside from the command buffer.

			//vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
			//instanceCount: Used for instanced rendering, use 1 if you're not doing that.
			//firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex
			//firstInstance: used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
			vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(m_CommandBuffers[i]);

			if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to record command buffers!");
		}

	}

	void DrawFrame()
	{
		//The function calls that get called in this method will return before the operations are actually finished,
		//and the order of execution is also undefined. That's unfortunate, because each of the operations depends on the previous one finishing.
		//There are two ways of synchronizing swap chain events: fences and semaphores. They're both objects that can be used
		//for coordinating operations by having one operation signal and another operation wait for a fence or semaphore to go from the unsignaled to signaled state.
		//The difference is that the state of fences can be accessed from your program using calls like vkWaitForFences and semaphores cannot be.
		//Fences are mainly designed to synchronize your aplicatoin itself with rendering operation, whereas semaphores are used to synchronize operations
		//within or across command queues.
		//We want to synchronize the queue operations of draw commands and presenation, which makes semaphores the best fit.

		//the first thing we need to do is acquire an image from the swap chain.
		uint32_t imageIndex;

		//The first 2 parameters of vkAcquireNextImageKHR are the logical device and the swap chain from which we wish to acquire an image.
		//The third parameter specifies a timeout in nanoseconds for an image to become visible.
		//using the maximum value of a 64 bit unsigned integer disables the timeout.
		//The next 2 parameters specify synchronization objects that are to be signaled when the presenation engine is finished using the image.
		//That's the point in time where we can start drawing to it. It is possible to specify a semaphore, fence or both.
		//We're going to use our m_ImageAvailableSemaphore for that purporse here.
		//The last parameter specifies a variable to output the index of the swap chain image that has become available.
		//The index refers to the VkImage in our m_SwapChainImages array. We're going to use that index to pick the right command buffer.

		vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		//the first 3 parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait.
		//We want to wait with writing colors to the image untill it's available, so we're specifying the stage of the graphics pipelines
		//that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex shader and such
		//while the image is not yet available, each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
		VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		//The next 2 parameters specify which command buffers to actually submit for execution. As mentioned earlier
		//We should submit the command buffer that binds the swap chain image we just acquired as color attachment.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

		//The signalSemaphoreCount and pSigneSemaphores parameters specify which semaphores to signal once the command buffer(s) have finished execution.
		//In our case we're using the m_RenderFinishedSemaphore for that purpose.
		VkSemaphore signalSemaphores[] = { m_RenderFinishedSamephore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		//We can now submit the command buffer to the graphics queue using vkQueueSubmit.
		//the function takes an array of VkSubmitInfo structues as argument for efficiency when the workload is much larger.
		//The last parameter references an optional fence that will be signaled when the command buffers finish execution.
		//We're using semaphores for synchronization, so we'll just pass a VK_NULL_HANDLE
		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
			throw std::runtime_error("failed to submit draw command buffer!");

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
		dependency.dstAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		//renderPassInfo.dependencyCount = 1;
		//renderPassInfo.pDependencies = &dependency;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		//The first 2 parameters specify which semaphores to wait on before presentation can happen, just like VkSubmitInfo.
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_SwapChain };

		//The next 3 parameters specify the swap chains to present images to and the index of the image for each swap chain.
		//This will almost always be a single one.
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		//There is one last optional parameter called pResults.
		//It allows you to specify an array of VkResult values to check for every individual swap chain if presentation was successful.
		//It's not necessary if you're only using a single swap chain, because you can simply use the return value of the present function.
		presentInfo.pResults = nullptr; //Optional

		//The vkQueuePresentKHR function submits the request to present an image to the swap chain.
		//We'll add error handling for both vkAcquireNextImageKHR and vkQeuuePresentKGR in the next chapter, because their failure
		//does not necessarily mean the program should terminate, unlike the functions we've seen so far
		vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	}

	void CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSamephore) != VK_SUCCESS)
			throw std::runtime_error("failed to create semaphores!");
	}

private:
	GLFWwindow* m_pWindow = nullptr;
	VkInstance m_Instance = nullptr;
	VkDebugUtilsMessengerEXT m_Callback;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device;
	VkQueue m_GraphicsQueue;
	VkSurfaceKHR m_Surface;
	VkQueue m_PresentQueue;
	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;
	std::vector<VkImageView> m_SwapChainImageViews;
	VkRenderPass m_RenderPass;
	VkPipelineLayout m_PipelineLayout = {};
	VkPipeline m_GraphicsPipeline;
	std::vector<VkFramebuffer> m_SwapChainFrameBuffers;
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkSemaphore m_ImageAvailableSemaphore;
	VkSemaphore m_RenderFinishedSamephore;


	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
	const bool m_EnableValidationLayers = false;
#else
	const bool m_EnableValidationLayers = true;
#endif // NDEBUG

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;
};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		std::cin.get();
		return EXIT_FAILURE;
	}

	std::cin.get();
	return EXIT_SUCCESS;
}