//GLFW Defines and includes
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//C++ includes
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>

//Exposes functions to do precise timekeeping.
#include <chrono>

//GLM Defines and includes
//GLM_FORCE_RADIANS definition is necessary to make sure that functions like glm::rotate use radians as arguments
//to avoid any possible confusion.
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

//Exposes functions that can be used to generate model transformations like glm::rotate, 
//view transformations like glm::lookat
//proj transformatoin like glm::perspective.
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

//Create different queue family specifically for transfer operatinos. It will require you the following changes:
//1) Modify QueueFamilyIndices and FindQueueFamilies to explicitly look for a queue family with the VK_QUEUE_TRANSFER bit,
//but not the VK_QUEUE_GRAPHICS_BIT.
//2) Modify CreateLogicalDevice to request a handle to the transfer queue
//3) Create a second command pool for command buffers that are submitted on the transfer queue family
//4) Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics and transfer queue families
//5) Submit any transfer command like vkCmdCopyBuffer to the transfer queue instead of the graphics queue

//Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
//Fixed-function state: all of the structures that define the fixed-functoins stages of the pipeline, like input assembly, rasterizer, viewport and color blending
//Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
//Render pass: the attachments referenced by the pipeline stages and their usage

/*
struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

Vertex Shader:
layout(binding = 0) unifomr UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0f, 1.0f);
	fragColor = inColor;
}
*/

//How to add a texture:
//Create an image object backed by device memory
//Fill it with pixels from an image file
//Create an image sampler
//Add a combined image sampler descriptor to sample colors from the texture

//One of the most common ways to transition the layout of an image is a pipeline barrier.
//Pipeline barriers are primarily used for synchrnizing access to resources, like making sure an image was
//written to before it is read, but they can also be used to transition layouts
//Barriers can additionally be used to transfer queue family ownership when using VK_SHARING_MODE_EXCLUSIVE

struct UniformBufferObject
{
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Proj;
};

struct Vertex
{
	glm::vec2 Position;
	glm::vec3 Color;
	glm::vec2 TexCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		//A vertex binding describes at which rate to load data from memory throught the vertices.
		//It specifies the number of bytes between data entries and wheter to move to the next data entry
		//after each vertex or after each instance.
		VkVertexInputBindingDescription bindingDescription = {};

		//all of our per-vertex data is packed together in one array, so we're only going to have one binding.
		//the binding paramter specifies the index of the binding in the array of bindings.
		//The stride parameters specifies the number of bytes from one entry to the next
		//the inputRate parameter can have one of the following values:
		//VK_VERTEX_INPUT_RATE_VERTEX: move to the next data entry after each vertex
		//VK_VERTEX_INPUT_RATE_INSTANCE: move to the next data entry after each instance.
		//we're not going to use instanced rendering, so we'll stick to per-vertex data.
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
		//The second structure that describes how to handle vertex input is VkVertexInputAttributeDescription.
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		//An attribute description struct describes how to extract a vertex attribute from a chuknk of vertex data.
		//originating from a binding description.
		//We have 2 attributes, position and color, so we need 2 attribute description structs

		//POSITION
		//------------------

		//Tells Vulkan from which binding the per-vertex data comes
		attributeDescriptions[0].binding = 0;
		//The location parameter references the location directive of the input in the vertex shader
		//The input in the vertex shader with location 0 is the position, which has 2 32-bit float components
		attributeDescriptions[0].location = 0;
		//The format parameter describes the type of data for the attribute.
		//A bit confusingly, the formata are specified using the same enumeration as color formats.
		//The following shader types and formats are commonly used together:
		//float: VK_FORMAT_R32_SFLOAT
		//vec2: VK_FORMAT_R32G32_SFLOAT
		//vec3: VK_FORMAT_R32G32B32_SFLOAT
		//vec4: VK_FORMAT_R32G32B32A32_SFLOAT
		//The color type (SFLOAT_, UINT, SINT) and bit width should also match the type of the shader input.
		//Examples:
		//ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
		//uvec4: VK_FORMAT_R32G32B32A32, a 4-component vector of 32-bit unsigned integers
		//double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		//specifies the number of bytes since the start of the per-vertex data to read from.
		//The bniding is loading one Vertex at a time and the position attribute(Position) i at an offset of 0 bytes
		//from the beginning of this struct. This is automatically calculated using the offsetof macro.
		attributeDescriptions[0].offset = offsetof(Vertex, Position);

		//COLOR
		//-----------------
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, Color);

		//TEXCOORD
		//-----------------
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

		return attributeDescriptions;
	}
};

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

		glfwSetWindowUserPointer(m_pWindow, this);
		glfwSetFramebufferSizeCallback(m_pWindow, FrameBufferResizeCallback);
	}

	void InitializeVulkan()
	{
		CreateInstance();
		ShowExtentions();
		SetupDebugCallback();
		CreateWindowSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffer();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(m_pWindow))
		{
			glfwPollEvents();
			DrawFrame();
		}

		vkDeviceWaitIdle(m_Device);
	}

	void Cleanup()
	{
		//Clean vulkan
		CleanupSwapChain();

		vkDestroySampler(m_Device, m_TextureSampler, nullptr);

		vkDestroyImageView(m_Device, m_TextureImageView, nullptr);

		vkDestroyImage(m_Device, m_TextureImage, nullptr);
		vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);

		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

		for (size_t i = 0; i < m_SwapChainImages.size(); ++i)
		{
			vkDestroyBuffer(m_Device, m_UniformBuffers[i], nullptr);
			vkFreeMemory(m_Device, m_UniformBuffersMemory[i], nullptr);
		}

		vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
		vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
		vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroyDevice(m_Device, nullptr);

		if (m_EnableValidationLayers)
			DestroyDebugUtilsMessengerEXT(m_Instance, m_Callback, nullptr);

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

	void ShowExtentions()
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
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(gpu, &deviceProps);
			
			std::cout << "Checking " << deviceProps.deviceName << std::endl;

			if (IsGpuSuitable(gpu))
			{
				m_PhysicalDevice = gpu;
				//break;
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

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
		deviceFeatures.samplerAnisotropy = VK_TRUE;

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
		vkGetDeviceQueue(m_Device, indices.GraphicsFamiliy, 0, &m_PresentQueue);
	}

	void CreateWindowSurface()
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
			int width, height;
			glfwGetFramebufferSize(m_pWindow, &width, &height);

			VkExtent2D actualtExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

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
			m_SwapChainImageViews[i] = CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat);
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

		VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::GetAttributeDescriptions();

		//The pVertexBindingDescriptions and pVertexAttributeDescriptions members point to an array
		//of structs that describe that aformentinoed details for loading vertex data.
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; //Optional
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); //Optional

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
		//Because we scale the Y scale axis by -1 we need to draw in counter clockwise order.
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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

		//We need to specifiy the descriptor set layout during pipeline creation to tell Vulkan which descriptors
		//the shaders will be using. Descriptor set layouts are specified in the pipeline layout objects.
		//Modify the VkPipelineLayoutCreateInfo to reference the layout object.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; //Optional
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout; //Optional
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

		//Others:
		//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: Optimal as source in a transfer operation
		//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: Optimal for sampling from a shader

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

			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			//The first 2 parameters, besides the command buffer, specify the offset and number of bindings
			//we're going to specify vertex buffers for. The last 2 paramets specify the array of vertex buffers
			//to bind and the byte offsets to start reading vertex data from.
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

			//An index buffer is bound with vkCmdBindIndexBuffer which has the index buffer, a byte offset into it,
			//and the type of the index data as parameters.
			//The possible types are VK_INDEX_TYPE_UINT16 and VK_INDEX_TYPE_UINT32
			vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
			//The actual vkCmdDraw function is a bit anti climactic, but it's so simple because of all the information we specified in advance.
			//It has the following parameters, aside from the command buffer.

			//Drawing without indices:
			//vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
			//instanceCount: Used for instanced rendering, use 1 if you're not doing that.
			//firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex
			//firstInstance: used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
			//vkCmdDraw(m_CommandBuffers[i], static_cast<uint32_t>(m_Vertices.size()), 1, 0, 0);

			//Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines.
			//Therefore we need to specify if we want to bind descriptor sets to the graphics or compute pipeline.
			//The next parameter is the layout that the descriptors are based on.
			//The next 3 parameters specify the index of the first descriptor set, the number of sets to bind and
			//the array of sets to bind.
			//The last 2 parameetres specify an array of offsets that are used for dynamic descriptors.
			vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[i], 0, nullptr);

			//A call to this funtion is very similar to vkCmdDraw. the first 2 parameters specify the number of indices
			//and the number of instances. We're not using instancing, so just specify 1 instance.
			//The number of indices represents the number of vertices that will bepassed to the vertex buffer.
			//The next parameter specifies an offset into the index buffer, using a value of 1 would cause the graphics card
			//to start reading at the second index. The second to last parameter speicifes an offset to add to the indices
			//in the index buffer. the final parameter specifies an offset for instancing, which we're not using.

			//Driver developers recommend that you also store mutliple buffers, like the vertex and index buffer,
			//into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers. The advantage is that your data
			//is more cache friendly in that case, because it's closer together. It is even possible to reuse the same chunk
			//of memory for multiple resources if they are not used during the same render operations, provided that their data
			//is refreshed, of course. This is known as aliasing and some Vulkan functions have explicit flags to specify that you 
			//want to do this.
			vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(m_CommandBuffers[i]);

			if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to record command buffers!");
		}

	}

	void DrawFrame()
	{
		//Wait for the frame to be finished
		//To learn more about synchrnoization through examples,
		//have a look at this extensive overview by Kronos
		//https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present

		//The vkWaitForFences function takes an array of fences and wait for either any or all of them to be signaled before returning.
		//The VK_TRUE we pass here indicates that we want to wait for all fences, but in the case of a single one it obviouslt doesn't matter.
		//just like vkAcquireNExtImageKHR this function also takes a timeout. 
		vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

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

		VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		//if the swap chain turns out to be out of date when attempting to qcquire an image,
		//then it is no longer possible to present it.
		//Therefore we should immediately recreate the swap chain and try again in the next DrawFrame call.
		//However, if we abort drawing at this point, then the fence will never have been submitted with vkQueueSubmit
		//and it'll be in an unexpected state when we try to wait for it later on.
		//We could recreate the fences as part of the swap chain recreation but it's easier to move the vkResetFences call
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("failed to acquire swap chain image!");

		UpdateUniformBuffer(imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		//the first 3 parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait.
		//We want to wait with writing colors to the image untill it's available, so we're specifying the stage of the graphics pipelines
		//that writes to the color attachment. That means that theoretically the implementation can already start executing our vertex shader and such
		//while the image is not yet available, each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
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
		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		//Unlike the semaphores, we manually need to restore the fence
		//to the unsignaled state by resetting it with the vkResetFence call.
		vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

		//We can now submit the command buffer to the graphics queue using vkQueueSubmit.
		//the function takes an array of VkSubmitInfo structues as argument for efficiency when the workload is much larger.
		//The last parameter references an optional fence that will be signaled when the command buffers finish execution.
		///Deprecated: We're using semaphores for synchronization, so we'll just pass a VK_NULL_HANDLE
		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
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
		//The vkQueuePresentKHR also return the same values as before. In this case we will also recreate the swap chain
		//if is is suboptimal, because we want the best possible result.
		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FrameBufferResized)
		{
			m_FrameBufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("failed to present swap chain image!");

		//If you run your application with validation layers enabled and you monitor the memory usage of your application,
		//you may notice that is is slowly growing. The reason for this is that the applicatoin is rapidly submitting work in the DrawFrame
		//function, but doesn't actually check if any of it finishes. If the CPU is submitting work faster than the GPU can keep up with then
		//the queue will slowly fill up with work. Worse, even, is that we are reusing the m_ImageAvailableSemaphores
		//and m_RenderFinishedSemaphore for multile frames at the same time.

		//The easy way to solve this is to wait for work to finish right after submitting it, for example by using vkQueueWaitIdle.
		vkQueueWaitIdle(m_PresentQueue);

		//However, we are likely not optimally using the GPU in this way, because the whole graphics pipeline is only
		//used for one frame at a time right now. The stages that the current frame has already progressed through are idle and
		//could already be used for a next frame.
		m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	//Create semaphores and fences
	void CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		//Initialize a fence in the signaled state as if we already rendered a frame
		//this is to make sure the VkWaitForFences doesn't wait for ever for the first frame to be rendered.
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
				throw std::runtime_error("failed to create synchronization objects for a frame!");

		}
	}

	void CleanupSwapChain()
	{
		for (size_t i = 0; i < m_SwapChainFrameBuffers.size(); ++i)
			vkDestroyFramebuffer(m_Device, m_SwapChainFrameBuffers[i], nullptr);

		//Instead of destroying the command pool, we just clean up the exisiting command buffers
		//with vkFreeCommandBuffers. This way we can reuse the existing pool to allocate the new command buffers.
		vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

		for (size_t i = 0; i < m_SwapChainImageViews.size(); ++i)
			vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	}

	void RecreateSwapChain()
	{
		int width = 0;
		int height = 0;

		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_pWindow, &width, &height);
			glfwWaitEvents();
		}

		//To make sure we don't touch any resources that are still in use,
		//We call vkDeviceWaitIdle first.
		vkDeviceWaitIdle(m_Device);

		//Clean up previous objects before recreating them
		CleanupSwapChain();

		//Recreate the swapchain itself
		CreateSwapChain();

		//the image views need to be recreated because they are based directly on the swap chain images
		CreateImageViews();

		//The render pass needs to be recreated because it depends on the format of the swap chain images.
		//it is rare for the swap chain image format to change during an operation like a window resize
		//but it should still be handled
		CreateRenderPass();

		//Viewport and scissor rectangles size is speciifed during graphics pipeline creation, so the pipeline
		//also needs to be rebuilt. it is possible to avoid this by using dynamic state for the viewports and scissor rectangles.
		CreateGraphicsPipeline();
		
		//The frame buffers and command buffers also directly depend on the swap chain images.
		CreateFrameBuffers();
		CreateCommandBuffers();
	}

	//The reason that we're creating a static function as a callback is because GLFW does not know how
	//to properly call a member function with the right this pointer to our HelloTriangleApplication instance.
	//However, we do get a reference to the GLFWwindow in the callback and there is another GLFW function that
	//allows you to store an arbitrary pointer inside of it: glfwSetWindowUserPointer
	static void FrameBufferResizeCallback(GLFWwindow* pWindow, int width, int height)
	{
		HelloTriangleApplication* pApp = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(pWindow));
		pApp->m_FrameBufferResized = true;
	}

	void CreateVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
		//CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertexBuffer, m_VertexBufferMemory);

		VkBuffer staginBuffer;
		VkDeviceMemory stagingBufferMemory;
		
		//We're now using a new staginBuffer with staginBufferMemory for mapping and copying the vertex data.
		//In this chapter we're going to use 2 new buffer flags:
		//VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
		//VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.

		//The m_VertexBuffer is now allocated from a memory type that is device local, which generally means
		//that we're not able to use vkMapMemory. However, we can copy data from the stagingBuffer to the m_VertexBuffer.
		//We have to indicate that we inted to do that by specifying the transfer source flag for the stagingBuffer and
		//the transfer destination flag for the m_VertexBuffer, along with the vrtex buffer usage flag.
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staginBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);
		CopyBuffer(staginBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(m_Device, staginBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

		//It should be noted that in real world applications, you're not supposed to actually call vkAllocateMemory
		//for every individual buffer. The Maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount
		//physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080.
		//The right way to allocate memory for a large number of objects at the same time is to create a custom allocator
		//that splits up a single allocatoin among many different objects by using the offset parameters that we've
		//seen in many functions.

		//You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiave.
		//However, for this tutorial it's okay to use a seperate allocation for every resource, because we won't come close
		//to hitting any of these limits for now.

		//Copy vertices into buffer
		//Unfortunately the driver may not immediately copy the data into the buffer memory.,
		//for example because of caching. It is also possible that writings to the buffer are not viisble
		//in the mapped memory yet. There are 2 ways to deal with that problem:
		//1) Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT.
		//2) Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges
		//before reading from the mapped memory.

		//We went for the first approach, which ensures that the mapped memory always matches the contents of the allocated memory.
		//Do keep in mind that this may lead to slightly worse performance than explicit flushing.
		//but we'll see why that doesn't matter in the next chapter (Vulkan Tutorial page 168)
		//void* data;
		//vkMapMemory(m_Device, m_VertexBufferMemory, 0, bufferSize, 0, &data);
		//memcpy(data, vertices.data(), (size_t)bufferSize);
		//vkUnmapMemory(m_Device, m_VertexBufferMemory);
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

		//The VkPhysicalDeviceMemoryProperties structure has 2 arrays memoryTypes and memoryHeaps.
		//Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for 
		//when VRAM runs out. The different types of memory exist within these heaps.
		//Right now we'll only concern ourselves with the type of memory and not the heap if comes from,
		//but you can image that this can affect performance.

		//Let's first find a memory type that suitable for the buffer itself:
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			//The typeFilter parameter will be used to specify the bit field of memory types that are suitable.
			//That means that we can find the index of a suitable memory type by simply iterating over them and
			//checking if the corresponding bit is set to 1

			//However, we're not just interested in a memory type that is suitable for the vertex buffer.
			//We also need to be able to write ouir vertex data to that memory.
			//The memoryTypes array consists of VkMemoryType structs that specify the heap and properties of each type of memory.
			//The properties define special features of the memory, like being able to map it so we can write it from the CPU.
			//This property is indicated with VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property.
			//We'll see why when we map the memory.

			//We may have more than one desirable property, so we should check if the result of the bitwise AND is not 
			//jsut non-zero, but equal to the desired properties bit field. IF there is a memory type suitable for the buffer
			//that also has all of the properties we need, then we return its index, otherwise we throw an exception.
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

		//Specifies the size of the buffer in bytes.
		//Calculating the byte size of the vertex data is straightfforward with sizeof.
		bufferInfo.size = size;

		//Indicates for which purpose the data in the buffer is goig to be used.
		//It is possible to specify multiple purposes using a bitwise or.
		//Our use case will be a vertex buffer.
		bufferInfo.usage = usage;

		//Just like the images in the swap chain, buffers can also be owned by a specific queue family or
		//be shared between multiple at the same time. The buffer will only be used from the fraphics queue,
		//so we can stick to exclusive access.
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//The flags parameter is used to configure sparase buffer memory, which is not relevenat right now.
		//We'll leave it at the default value of 0.


		if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create buffer!");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

		//The VkMemoryREquirements struct has 3 fields:
		//size: The size of the required amount of memory in bytes, may differ from bufferInfo.size
		//alignment: The offset in bytes where the buffer begins in the allocated region of memory,
		//depends on bufferInfo.usage and bufferInfo.flags
		//MemoryTypeBits: Bit field of the memory types that are suitable for the buffer

		//Graphics cards can offer different types of memory to allocate from. Each type of memory varies
		//in terms of allowed operations and performance characteristics.
		//We need to combine the requirements of the buffer and our own application requirements to find the right
		//type of memory to use.

		//Memory allocatoin is now as simple as specifying the size and type, both of which are derived
		//from the memory requirements of the vertex buffer and the desired property.
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate buffer memory!");

		//the fourth parameter is the offset within the region of memory.
		//Since this memory is allocated specifically for this vertex buffer, the offset is simply 0.
		//If the offset is non-zero, then it is required to be divisble by memREquirements.alignment.
		vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
	}

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0; //Optional
		copyRegion.dstOffset = 0; //Optional
		copyRegion.size = size;

		//Contents of buffers are transferred using the vkCmdCopyBuffer command.
		//It takes the source and destination buffers as arguemtns, and an array of regions to copy.
		//The regions are defined in VkBufferCopy structs and consist of a soruce buffer offset, 
		//destinaiton buffer offset and size. //It is not possible to specify VK_WHOLE_SIZE here, unlike the vkMapMemory command.
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer);
	}

	void CreateIndexBuffer()
	{
		//There are only 2 notable idfferences. The bufferSize is now equal to the number of indices times the size of the index type,
		//either uint16_t or uint32_t. The usage of the m_IndexBuffer should be VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead
		//of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, which makes sense. Other than that, the process is exactly the same.
		//We create a staging buffer to copy the contents of m_Indices to and then copy it to the final device local index buffer.
		VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

		CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	void CreateDescriptorSetLayout()
	{
		//Every binding needs to be described through a VkDescriptorSetLayoutBinding
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};

		//The first 2 fields specify the binding used in the shader and the type of descriptor,
		//which is a uniform buffer object.
		//It is possible for the shader variable to represent an array of uniform buffer objects,
		//and descriptorCount specifies the number of values in the array. 
		//This could be used to specify a transformatoin for each of the bones in a skeleton for skeletal animation, for example.
		//Our MVP transformation is in a single uniform buffer object, so we're using a descriptorCount of 1.
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;

		//We also need to specify in which shader stages the descriptor is going to be referenced.
		//The stageFlags field can be a combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS.
		//In our case, we're only referencing the descriptor from the vertex shader.
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		//The pImmutableSamplers field is only relevant for image sampling related descriptors.
		uboLayoutBinding.pImmutableSamplers = nullptr; //Optional
		
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		//Make sure to set the stageFlags to indicate that we intend to use the combinded image sampler
		//descriptor in the fragment shader. That's where the color of the fragment is going to be determined.
		//It is possible to use texture sampling in the vertex shader, for example to dynamically deform a grid
		//of vertices by a heightmap.
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		//All of the descriptor binding are combinded into a single VkDescriptorSetLayout object.
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		
		if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");
	}

	void CreateUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(m_SwapChainImages.size());
		m_UniformBuffersMemory.resize(m_SwapChainImages.size());

		for (size_t i = 0; i < m_SwapChainImages.size(); ++i)
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);

	}

	void UpdateUniformBuffer(uint32_t currentImage)
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
		vkMapMemory(m_Device, m_UniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_Device, m_UniformBuffersMemory[currentImage]);

	}

	void CreateDescriptorPool()
	{
		//We first need to describe which descriptor types our descriptor sets are going to contain and
		//how many of them, using VkDescriptorPoolSize structures
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(m_SwapChainImages.size());

		//We will allocate one of these descriptors for every frame. This pool size structure is referenced
		//ny the main VkDescriptorPoolCreateInfo:
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		//Aside from the maimum number of individual descriptors that are available,
		//We also need to specify the maimum number of descriptors sets that may be allocated.
		poolInfo.maxSets = static_cast<uint32_t>(m_SwapChainImages.size());

		//The structure has an optional flag similar to command pools that determines if individual
		//descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
		//We're not going to touch the descriptor set after creating it, so we don't need this flag.

		if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor pool!");

	}

	void CreateDescriptorSets()
	{
		//A descriptor set allocatoin is described with a VkDescriptorSetAllocateInfo struct.
		//You need to specify the desriptor pool to allocate from, the number of descriptors sets to allocate
		//and the descriptor layour to base them on.

		std::vector<VkDescriptorSetLayout> layouts(m_SwapChainImages.size(), m_DescriptorSetLayout);
		
		//In our case, we will create one descriptor set for each swap chain image, all with the same layout.
		//Unfortunately we do need all the copies of the layour because the next function expects an array matching the numbers of sets.
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(m_SwapChainImages.size());

		//You don't need to explicitly clean up descriptor sets, because they will be automatically freed
		//when the desriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor
		//sets, each with one uniform buffer descriptor.
		if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_DescriptorSets[0]) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate descriptor sets!");


		//The descriptor sets have been allocated now, but the descriptors within still need to be configured.
		for (size_t i = 0; i < m_SwapChainImages.size(); ++i)
		{
			//Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a vkDescriptorBufferInfo.
			//This structure specifies the buffer and the region within it that contains the data for the descriptor.
			VkDescriptorBufferInfo  bufferInfo = {};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			//If you're overwriting the whole buffer, like we are in this case, then it is also possible to use the VK_WHOLE_SIZE
			//value for the range. The configuration of descriptors is updated using the vkUpdateDescriptorsSets function,
			//which takes an array of VkWriteDescriptorSet structs as parameter.
			bufferInfo.range = sizeof(UniformBufferObject);

			//Bind the actual image and sampler resources to the descriptors in the descriptor set.

			//The resources for a combinded image sampler structure must be specified in a VkDescriptorImageInfo struct,
			//just like the buffer resource for a uniform buffer descriptor is specified in a VkDescriptorBufferInfo struct.
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_TextureImageView;
			imageInfo.sampler = m_TextureSampler;

			//The first 2 fields specify the descriptor set to update and the binding.
			//We gave our uniform buffer binding index 0. Remember that descriptors can be arrays, 
			//so we also need to specify the first index in the array that we want to update.
			//We're not using an array, so the index is simply 0.
			std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_DescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;

			//We need to specify the type of descriptor again. It's possible to update multiple descriptors
			//at once in an array, starrting at index dstArrayElement. The descriptorCount field specifies
			//how many array elements you want to update.
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			//pBufferInfo references an array with descriptorCount structs that actually configure the descriptors.
			//It depends on the type of descriptor which one of the three you actually need to use.
			//The pBufferInfo field is usedfor descriptors that refer to buffer data, pImageInfo is used for descriptors
			//that refer to image data, and pTextBufferView is used for descriptors that refer to buffer views.
			//Our descriptor is based on buffers, so we're using pBufferInfo.
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr; //Optional
			descriptorWrites[0].pTexelBufferView = nullptr; //Optional

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_DescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;

			//The descriptors must be updated with this image info, just like the buffer
			//This time we're using the pImageInfo array instead of pBufferinfo.
			//The descriptors are now ready to bused by the shaders!
			descriptorWrites[1].pImageInfo = &imageInfo;
			
			//The updates are applied using vkUpdateDescriptorSets.
			//It accepts two kinds of arrays as parameters:
			//An array of VkWriteDescriptorSet
			//An array of VkCopyDescriptorSet.
			//The latter can be used to copy descriptors to each other, as its name implies.
			vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		//As some of the structues and function calls hinted at,
		//it is actually possible to bind multiple descriptor sets simultaneously.
		//You need to specify a descriptor layour for each descriptor set when creating
		//the pipeline layout. Shaders can then reference specific desciptor sets like this:
		//layout(set = 0, binding = 0) uniform UniformBufferObject { ... }

		//You can use this feature to put descriptors that vary per-object and descriptors that are
		//shared into separate descriptor sets. In that case you avoid rebinding most of the descriptors 
		//across draw call which is potentially more efficient.

	}

	void CreateTextureImage()
	{
		int texWidth, texHeight, texChannels;

		//the stbi_load function takes the file path and number of channels as arguments.
		//the STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn't have one,
		//which is nice for consistency with other textures in the future.
		//The middle 3 parameters are outputs for the dith, height and actual number of channels in the image.
		//The pointer that is returned is the first elemtn in an array of pixel values.
		//The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
		stbi_uc* pixels = stbi_load("../data/textures/chief.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4; //4 -> rgba

		if (!pixels)
			throw std::runtime_error("failed to load texture image!");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		
		//The buffer should be in host visible memory so that we can map it
		//and it should be usable as a transfer source so that we can copy it to an image later on.
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(m_Device, stagingBufferMemory);

		//clean up the original pixel array
		stbi_image_free(pixels);

		CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

		//Copy the staging buffer to the texture image with 2 steps:
		//Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		//Execute the buffer to image copy operation

		//The image was created with the VK_IMAGE_LAYOUT_UNDEFFINED layout, so that one should be specified as old layout 
		//when transitioning textureImage.
		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		//To be able to start sampling from the texture image in the shader, we need one last transition
		//to prepare it for shader access.
		TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

	}	
	
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		//Tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed.
		//It is possible to create 1D, 2D and 3D images.
		//1D images can be used to store an array of data or gradient, 2D images are mainly used for textures
		//3D images can be used to store voxel volumes, for example.
		imageInfo.imageType = VK_IMAGE_TYPE_2D;

		//specifies the dimensions of the image, basically how many texels there are on each axis.
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;

		//must be 1, not 0
		imageInfo.extent.depth = 1;

		//no mip mapping or arrays, for now.
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		
		//Vulkan supports many possible image formats, but we should use the same format for the texels as the pixels in the buffer,
		//otherwise copy operation will fail.
		imageInfo.format = format;

		//The tiling field can have one of the two values:
		//VK_IMAGE_TILING_LINEAR: Texetls are laid out in row-major order like our pixels array
		//VK_IMAGe_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
		imageInfo.tiling = tiling;
		
		//Unlike the layout of an image, the tiling mode cannot be changed at a later time.
		//If you want to be able to directly access texels in the memory of the image, then you must use VK_IMAGE_TILING_LINEAR.

		//There are only 2 possible values for the initialLayout of an image:
		//VK_IMAGE_LAYOUT_UNDEFINED: Not usable by the GPU and the very first transition will discard the texels.
		//VK_IMAGE_LAYOUT_PREINITIALIZED: Not usable by the GPU, but the first transition will preserve the texels
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		//There are a few situations where it is necessary for the texels to be preserved during the first transition.
		//One example, however, would be if you wanted to use an image as a staging image in combination with the VK_IMAGE_TILING_LINEAR layout
		//In that case you'd want to upload the texel data to it and then transition the image to be a transfer source wihtout losing the data.
		//In our case, however, we're first going to transition the image to be atransfer destination and 
		//then copy texel data to it from a buffer object so we don't need this property and can safely use VK_IMAGE_LAYOUT_UNDEFINED

		//The usage field has the same semantics as the one during buffer creation. The image is going to be used as
		//destination for the buffer copy, so it should be set up as a trnasfer destination. We also want to be able to access the image
		//from the shader to color our mesh, so the usage should include VK_IMAGE_USAGE_SAMPLED_BIT.
		imageInfo.usage = usage;

		//The image will only be used by one queue family: the one that supports graphics (and therefore also) transfer operations.
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//The samples flag is related to multisampling. This is only relevant for images that will be used as attachments,
		//So stick to one sample. There are some optional flags for images that are related to sparse images. Sparse images are images
		//where only certain regions are actually backed by memory. If you were using a 3D texture for a voxel terrain,
		//for example, then you could use this to avoid allocating memory to store large volumes of "air" values.
		//We won't be using it in this tutorial, so leave it to its default 0 value.
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; //Optional

		if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
			throw std::runtime_error("failed to create image!");

		//Same as buffer allocation except use vkGetImageMemoryRequirements and vkBindImageMemory
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
			throw std::runtime_error("failed to allocate image memory!");

		vkBindImageMemory(m_Device, image, imageMemory, 0);
	}

	VkCommandBuffer BeginSingleTimeCommands()
	{
		//Memory transfer operations are executed using command buffers, just like drawing commands.
		//Therefore we must first allocate a temporary command buffer.
		//You may wish to create a separate command pool for these kinds of short-lived buffers, 
		//because the implementation may be able to apply memory allocation optimizations.
		//You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool genreration in that case.
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

		//Immediatly start recording the command buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//The VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag that we used for the drawing command buffers
		//is not necessary here, because we're only going to use the command buffer once and wait with returning 
		//from the function until the copy operation has finished executing. It's good practice to tell the drive
		//about our intent using VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT.
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void EndSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//Unlike the draw commands, there are no events we need to wait on this time.
		//We just want to execute the transfer on the buffers immediatly.
		//There are again 2 possible ways to wait on this transfer to complete.
		//We could use a fence and wait with vkWaitForFences, or simply wait for the transfer queue to 
		//become idle with vkQueueWaitIdle.
		//A fence would allow you to schedule multiple transfers simultaneously and wait for all of them to complete,
		//Instead of executing one at a time. That may give the driver more opportunities to optimize.
		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_GraphicsQueue);

		vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
	}

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		//One of the most common ways to perform layout transitions is using an image memory barrier.
		//A pipeline barrier like that is generally used to synchronize access to resources,
		//like ensuring that a write to a buffer completes before reading from it, but it can also be used
		//to transition image layours and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used.
		//there is an equivalent buffer memory barrier to do this for buffers
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		//The first 2 fields specify layout transition. It is possible to use VK_IMAGE_LAYOUT_UNDEFINED
		//as oldLayout if you don't care about the existing contents of the image.
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		//if you are using the barrier to transfer queue family ownership, then these two fields should
		//be the indices of the queue families. They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want
		//to do this (not the default value!).
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		//The image and subresourceRange specify the image that is affected and the specific part of the image.
		//Our image is not an array and does not have mipmapping levels, so only one level and layer are specified.
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		//Barriers are primarily used for synchronization purposes, so you must specify which types of operations
		//that involve the resource must happen before the barrier, and which operations that involve the resource
		//must wait on the barrier. We need to do that despite already using vkQueueWaitIdle to manually synchronize.
		//The right values depend on the old and new layout, so we'll get back to this once we've figured out which transitions
		//we're going to use.
		barrier.srcAccessMask = 0; //TODO
		barrier.dstAccessMask = 0; //TODO

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		//Validate access masks and pipeline stages in trnasitionImageLayout.
		//There are 2 that need to be set based on the layouts in the transition.
		//Undefined --> transfer destination: transfer writes that don't need to wait on anything
		//transfer destination --> shader reading: shader reads should wait on transfer writes, 
		//specifically the shader reads in the fragment shader, because that's where we're going to use the texture.

		//Since the writes don't have to wait on anythign, you may specify an empty access mask and 
		//the earliest possible pipeline stage VK_PIPELINE_sTAGE_TOP_OF_PIPE_BIT for the pre-barrier operations.
		//It should be noted that VK_PIPELINE_STAGE_TRANSFER_BIT is not a read stage within the graphics and compute pipelines.
		//It is more of a pseudo-stage where transfers happen. 
		//See the documentatoin for more information and other examples of pseudo-stages.
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		//The image will be written in the same pipeline stage and subsequently read by the fragment shader,
		//which is why we specify shader reading access in the fragment shader pipeline stage.
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		else
			throw std::runtime_error("unsupported layout transition!");

		//All types of pipeline barriers are submitted using the same function.
		//The first parameter after the command buffer speicifes in which pipeline stage the operations occur
		//that should happen before the barrier. The pipeline stages that you are allowed to specify before and after
		//the barrier depend on how you use the resource before and after the barrier. The allowed values are listed here:
		//https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported
		//For example, if you're going to read from a uniform after the barrier, you would specify a usage of VK_ACCESSS_UNIFORM_READ_BIT
		//and the earliest shader that will read from the unifrom as pipeline stage, for example VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT.
		//It would not make sense to specify a no-shader pipeline stage for this type of usage and the validation layers
		//will warn you when you specify a pipeline stage that does not match the type of usage.
		
		//The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT. the latter turns the barrier into a per-region condition.
		//That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far, for example.
		
		//The last 3 pairs of parameters reference arrays of pipeline barriers of the 3 available types:
		//memory barriers, buffer memory barriers and image memory barriers like the one we're using here.
		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		EndSingleTimeCommands(commandBuffer);
	}

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		//Just like with buffer copies, you need to specify which part of is going to be copied to which part of the image.
		//This happens through VkBufferImageCopy structs.
		VkBufferImageCopy region = {};

		//Specifies the byte offset in the buffer at which the pixel values start.
		region.bufferOffset = 0;

		//specify how the pixels are laid out in memory.
		//For example, you could have some padding bytes between rows of the image.
		//Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case.
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		//indicates which part of the image we want to copy the pixels
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0,0,0 };
		region.imageExtent = { width, height, 1 };

		//the fourth parameters indicates which layout the image is currently using.
		//I'm assuming here that the image has already been transitioned to the layout that is optimal for copying
		//pixels to.
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		EndSingleTimeCommands(commandBuffer);
	}

	void CreateTextureImageView()
	{
		//The code for this function can be based directly on CreateImageViews.
		//The only 2 changes you have to make are the format and the image
		m_TextureImageView = CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM);
	}

	VkImageView CreateImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		
		//The viewType and format fields specify how the image data should be interpreted.
		//the viewType parameter allows you to treat images as 1D textures, 2D textures, 3D textures and cubemaps
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		//The components field allows you to swizzle the color channels around. 
		//for example, you can map all of the channels to the red channel for a monochrome texture.
		//you can also map constant values of 0 and 1 to a channel. In our case we'll stick to the default mapping.
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//the subresourceRange field describes what the image's purpose is and which part of the image should be accessed.
		//Our images will be used as color targets without any mipmapping levels or multiple layers.
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture image view!");

		return imageView;
	}

	void CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		//The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified.
		//Magnification concerns the oversampling problem, minification concerns undersampling.
		//The choices are VK_FILTER_NEAREST and VK_FILTER_LINEAR.
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		//The addressing mode can be specified per axis using the addressMode fields.
		//The available values are: 
		//VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
		//VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repear, but inverts the coordinates to mirror the image when going beyond the dimensions.
		//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
		//VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
		//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Returns a solid color when sampling beyond the dimensions of the image.
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		//These 2 fields specify if anisotropic filtering should be used.
		//There is no reason not use this unless performance is a concern.
		//The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color.
		//A lower value results in better performance, but lower quality results.
		//There is no graphics hardware available today that will use more than 16 samples, because the difference 
		//is negligible beyond that point.
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;

		//Disable antisotropic filtering
		//samplerInfo.anisotropyEnable = VK_FALSE;
		//samplerInfo.maxAnisotropy = 1;

		//The borderColor specifies which color is returned when sampling beyond the image with clamp to border addressing mode.
		//it is possible to return black, white, transparent in either float or int formats. You cannot specify an arbitrary color.
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		//The unnormalizedCoordinates filed specifies which coordinate system you want to use to address texels in an image.
		//If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth] and [0, texHeight] range.
		//if is is VK_FALSE, then the texesl are addressed using the [0,1] range on all axes.
		//Real-World applications almost always use normalized coordinates, because then it's possible to use textures of varying
		//resolutions with the exact same coordinates.
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		//if a comparison function is enabled, then texels will first be compared to a value and
		//the result of that comparison is used in filtering operations. This is mainly used for percentage-closer
		//filtering on shadow maps.
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		//All of these fields apply to mipmapping. We will look at mipmapping in a later chapter, 
		//but basically it's another type of filter that can be applied.
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		//Note that the sampler does not reference a VkImage anywhere. The sampler is a distinct object
		//that provides an interface to extract colors from a texture.
		//It can be applied to any image you want, whether it is 1D, 2D or 3D. This is different from
		//many older APIs, which combinded texture images and filtering into a single state.
		if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
			throw std::runtime_error("failed to create texture sampler!");

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
	//VkSemaphore m_ImageAvailableSemaphore;
	//VkSemaphore m_RenderFinishedSemaphore;
	//Each frame should have its own set of semaphores
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	size_t m_CurrentFrame = 0;
	bool m_FrameBufferResized = false;
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	//Just like the vertex data, the indices need to be uploaded into a VkBuffer for the GPU to be able to access them.
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VkDeviceMemory> m_UniformBuffersMemory;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	

	const int MAX_FRAMES_IN_FLIGHT = 2;
	
	//Interleaving vertex attributes: all vertices and their attributes are defined in 1 buffer
	const std::vector<Vertex> m_Vertices = 
	{
		//	Position			Color			TexCoord
		//----------------------------------------------------
		{{-0.5f, -0.5f},  {1.0f, 0.0f, 0.0f},  {1.0f, 0.0f}},
		{{ 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f},  {0.0f, 0.0f}},
		{{ 0.5f,  0.5f},  {0.0f, 0.0f, 1.0f},  {0.0f, 1.0f}},
		{{-0.5f,  0.5f},  {1.0f, 1.0f, 1.0f},  {1.0f, 1.0f}}
	};

	/*
	-------
	|\    |
	| \ 1 |
	|  \  |
	| 2 \ |
	|    \|
	-------
	*/

	//It is possible to use either uin16_t or uin32_t for your index buffer depending on the number
	//of entries in vertices. We can stick to uint16_t for now because we're using less than 65534 unique vertices
	const std::vector<uint16_t> m_Indices = 
	{
		0,1,2, //top right
		2,3,0  //bottom left
	};

#ifdef NDEBUG
	const bool m_EnableValidationLayers = false;
#else
	const bool m_EnableValidationLayers = true;
#endif // NDEBUG

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;
};

int program()
{
	HelloTriangleApplication app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main()
{
	int errCode = program();
	std::cin.get();
	return errCode;
}