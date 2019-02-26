#include "DescriptorSetLayout.h"

#include <array>

#include "LogicalDevice.h"

DescriptorSetLayout::DescriptorSetLayout(LogicalDevice* pCpu):
	m_pCpu(pCpu)
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

	if (vkCreateDescriptorSetLayout(pCpu->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor set layout!");
}

DescriptorSetLayout::~DescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_pCpu->GetDevice(), m_DescriptorSetLayout, nullptr);
}