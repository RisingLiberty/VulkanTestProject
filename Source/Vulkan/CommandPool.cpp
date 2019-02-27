#include "CommandPool.h"

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GraphicsPipeline.h"
#include "PipelineLayout.h"

CommandPool::CommandPool(LogicalDevice* pCpu, PhysicalDevice* pGpu):
	m_pCpu(pCpu)
{
	const QueueFamilyIndices queueFamilyIndices = pGpu->GetDesc().QueueIndices;

	//Command buffers are executed by submitting them on one of the device queues, like the graphics and presentation queues we retrieved.
	//Each command pool can only allocate command buffers that are submitted on a single type of queue.
	//We're going to record command for drawing, which is why we've chosen the graphics queue family.

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;

	//there are two possible flags fro command pools:
	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: hint that command buffers are rerecorded with new command very often
	//This may change memory allocatoin behaviour)

	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to be rerecorded individaully, without this flag they all have to be reset together.

	//We will only record the command buffers at the beginning of the program and then execute them many times
	//in the main loop, so we're not going to use either of these flags.
	poolInfo.flags = 0; //Optional

	if (vkCreateCommandPool(pCpu->GetDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
		throw std::runtime_error("failed to create command pool!");
}

CommandPool::~CommandPool()
{
	//Instead of destroying the command pool, we just clean up the exisiting command buffers
	//with vkFreeCommandBuffers. This way we can reuse the existing pool to allocate the new command buffers.
	vkFreeCommandBuffers(m_pCpu->GetDevice(), m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

	vkDestroyCommandPool(m_pCpu->GetDevice(), m_CommandPool, nullptr);
}

void CommandPool::CreateCommandBuffers(RenderPass* pRenderPass, SwapChain* pSwapChain, VertexBuffer* pVertexBuffer, IndexBuffer* pIndexBuffer, GraphicsPipeline* pGraphicsPipeline, const std::vector<VkDescriptorSet>& descriptorSets)
{
	m_CommandBuffers.resize(pSwapChain->GetFrameBuffers().size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;

	//The level parameter specifies if the allocated command buffers are primary or secondary command buffers.
	//VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for exection, but cannot be called from other command buffers.
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_pCpu->GetDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
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
		renderPassInfo.renderPass = pRenderPass->GetRenderPass();
		renderPassInfo.framebuffer = pSwapChain->GetFrameBuffers()[i];

		//the render area defines where shader loads and stores will take place.
		//The pixels outside this region will have undefined values. It should match the size of the attachments for best performance.
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = pSwapChain->GetExtent();

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

		//the range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the far view plane and
		// 0.0 at the near view plane. The initial value at each point in the depth buffer should be the furthest
		//possible depth, which is 1.0
		clearValues[1].depthStencil = { 1.0f, 0 };

		//These parameters define the clear value to use for VK_ATTACHMENT_LOAD_OP_CLEAR
		//which we used as load operation for the color attachment.
		//I've defined the clear color to simply be black 100% opacity
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		//VK_SUBPASS_COONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself 
		//and no secondary dommand will be executed

		//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands iwll be executed from secondary command buffers.
		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->GetPipeline());

		VkBuffer vertexBuffers[] = { pVertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };

		//The first 2 parameters, besides the command buffer, specify the offset and number of bindings
		//we're going to specify vertex buffers for. The last 2 paramets specify the array of vertex buffers
		//to bind and the byte offsets to start reading vertex data from.
		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

		//An index buffer is bound with vkCmdBindIndexBuffer which has the index buffer, a byte offset into it,
		//and the type of the index data as parameters.
		//The possible types are VK_INDEX_TYPE_UINT16 and VK_INDEX_TYPE_UINT32
		vkCmdBindIndexBuffer(m_CommandBuffers[i], pIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
		vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->GetLayout()->GetPipelineLayout(), 0, 1, &descriptorSets[i], 0, nullptr);

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
		vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(pIndexBuffer->GetNrOfIndices()), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_CommandBuffers[i]);

		if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to record command buffers!");
	}
}