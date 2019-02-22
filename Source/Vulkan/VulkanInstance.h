#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanInstance
{
public:
	VulkanInstance(bool enableValidationLayers);
	~VulkanInstance();

	VkInstance GetInstance() const { return m_Instance; }
	const std::vector<const char*>& GetRequiredExtensions() const { return m_RequiredExtentions; }
	bool AreValidationLayersEnabled() const { return m_EnableValidationLayers; }

private:
	bool CheckValidationLayerSupport() const;
	void InitRequiredExtentions();
	void ShowExtensions() const;
	void SetupDebugCallback();
	VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pCallback);
	void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT callback);
	VkApplicationInfo MakeAppInfo() const;
	VkInstanceCreateInfo MakeInstanceCreateInfo();

private:
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_Callback;
	std::vector<const char*> m_RequiredExtentions;

#ifdef NDEBUG
	const bool m_EnableValidationLayers = false;
#else
	const bool m_EnableValidationLayers = true;
#endif
	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };
};