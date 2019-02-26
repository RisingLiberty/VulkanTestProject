#include "ShaderModule.h"

#include "LogicalDevice.h"

ShaderModule::ShaderModule(LogicalDevice* pCpu, const std::vector<char>& code):
	m_pCpu(pCpu)
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

	if (vkCreateShaderModule(pCpu->GetDevice(), &createInfo, nullptr, &m_Module) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module!");
}

ShaderModule::~ShaderModule()
{
	vkDestroyShaderModule(m_pCpu->GetDevice(), m_Module, nullptr);
}
