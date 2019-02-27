#include "DescriptorPool.h"

#include <array>

#include "LogicalDevice.h"
#include "SwapChain.h"
#include "DescriptorSetLayout.h"
#include "TextureSampler.h"
#include "Texture.h"


DescriptorPool::DescriptorPool(LogicalDevice* pCpu, size_t nrOfSwapChainImages):
m_pCpu(pCpu)
{
	//We first need to describe which descriptor types our descriptor sets are going to contain and
	//how many of them, using VkDescriptorPoolSize structures
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(nrOfSwapChainImages);

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(nrOfSwapChainImages);

	//We will allocate one of these descriptors for every frame. This pool size structure is referenced
	//ny the main VkDescriptorPoolCreateInfo:
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	//Aside from the maimum number of individual descriptors that are available,
	//We also need to specify the maimum number of descriptors sets that may be allocated.
	poolInfo.maxSets = static_cast<uint32_t>(nrOfSwapChainImages);

	//The structure has an optional flag similar to command pools that determines if individual
	//descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
	//We're not going to touch the descriptor set after creating it, so we don't need this flag.

	if (vkCreateDescriptorPool(pCpu->GetDevice(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
		throw std::runtime_error("failed to create descriptor pool!");

}

void DescriptorPool::CreateDescriptorSets(SwapChain* pSwapChain, DescriptorSetLayout* pDescSetLayout, TextureSampler* pSampler, Texture* pTexture)
{
	//A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct.
	//You need to specify the desriptor pool to allocate from, the number of descriptors sets to allocate
	//and the descriptor layour to base them on.

	std::vector<VkDescriptorSetLayout> layouts(pSwapChain->GetImages().size(), pDescSetLayout->GetDescriptorSetLayout());

	//In our case, we will create one descriptor set for each swap chain image, all with the same layout.
	//Unfortunately we do need all the copies of the layour because the next function expects an array matching the numbers of sets.
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_Pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(pSwapChain->GetImages().size());
	allocInfo.pSetLayouts = layouts.data();

	m_DescriptorSets.resize(pSwapChain->GetImages().size());

	//You don't need to explicitly clean up descriptor sets, because they will be automatically freed
	//when the desriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor
	//sets, each with one uniform buffer descriptor.
	if (vkAllocateDescriptorSets(m_pCpu->GetDevice(), &allocInfo, &m_DescriptorSets[0]) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate descriptor sets!");


	//The descriptor sets have been allocated now, but the descriptors within still need to be configured.
	for (size_t i = 0; i < pSwapChain->GetImages().size(); ++i)
	{
		//Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a vkDescriptorBufferInfo.
		//This structure specifies the buffer and the region within it that contains the data for the descriptor.
		VkDescriptorBufferInfo  bufferInfo = {};
		bufferInfo.buffer = pSwapChain->GetUniformBuffers()[i];
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
		imageInfo.imageView = pTexture->GetImageView();
		imageInfo.sampler = pSampler->GetSampler();

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
		vkUpdateDescriptorSets(m_pCpu->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(m_pCpu->GetDevice(), m_Pool, nullptr);
}