#include "PipelineLayout.h"

#include "LogicalDevice.h"
#include "DescriptorSetLayout.h"

PipelineLayout::PipelineLayout(LogicalDevice* pCpu, DescriptorSetLayout* pDescSetLayout)
{
	//We need to specifiy the descriptor set layout during pipeline creation to tell Vulkan which descriptors
	//the shaders will be using. Descriptor set layouts are specified in the pipeline layout objects.
	//Modify the VkPipelineLayoutCreateInfo to reference the layout object.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; //Optional
	pipelineLayoutInfo.pSetLayouts = &pDescSetLayout->GetDescriptorSetLayout(); //Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; //Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; //Optional

	if (vkCreatePipelineLayout(pCpu->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("failed to create pipeline layout");
}

