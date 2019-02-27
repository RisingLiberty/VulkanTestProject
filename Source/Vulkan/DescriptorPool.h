#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

class LogicalDevice;
class SwapChain;
class DescriptorSetLayout;
class TextureSampler;
class Texture;



class DescriptorPool
{
public:
	DescriptorPool(LogicalDevice* pCpu, size_t nrOfSwapChainImages);
	~DescriptorPool();

	void CreateDescriptorSets(SwapChain* pSwapChain, DescriptorSetLayout* pDescSetLayout, TextureSampler* pSampler, Texture* pTexture);

	const VkDescriptorPool& GetPool() const { return m_Pool; }
	const std::vector<VkDescriptorSet>& GetSets() const { return m_DescriptorSets; }
	
private:
	VkDescriptorPool m_Pool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	LogicalDevice* m_pCpu;
};