#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <set>
#include <vector>

class Surface;

struct QueueFamilyIndices
{
	int GraphicsFamily = -1;
	int PresentFamily = -1;

	bool IsComplete() const { return GraphicsFamily >= 0 && PresentFamily >= 0; }
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

struct PhysicalDeviceDesc
{
	VkPhysicalDeviceProperties Properties;
	VkPhysicalDeviceFeatures Features;
	std::vector<VkQueueFamilyProperties> QueueFamilies;
	QueueFamilyIndices QueueIndices;
	std::vector<VkExtensionProperties> AvailableExtensions;
	SwapChainSupportDetails SwapChainSupportDetails;
	VkPhysicalDeviceMemoryProperties MemProperties;
};

class PhysicalDevice
{
public:
	PhysicalDevice(Surface* pSurface, const VkPhysicalDevice& device, std::set<std::string>& requiredExtensions);
	~PhysicalDevice();

	bool IsSuitable() const;
	bool CheckDeviceExtensionSupport(std::set<std::string>& requiredExtensions) const;
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;
	int RateSuitability() const;

	VkPhysicalDevice GetDevice() const { return m_Device; }
	PhysicalDeviceDesc& GetDescRef() { return m_Desc; }
	const PhysicalDeviceDesc GetDesc() const { return m_Desc; }

private:
	void Initialize();

	//This function will get the indices of the queue families that satisfy certain desired properties.
	QueueFamilyIndices FindQueueFamilies(Surface* pSurface) const;
	std::vector<VkExtensionProperties> FindExtentions() const;
	SwapChainSupportDetails FindSwapChainSupport() const;

private:
	VkPhysicalDevice m_Device;
	Surface* m_pSurface;
	PhysicalDeviceDesc m_Desc;
	std::set<std::string>& m_RequiredExtensions;
	
};

