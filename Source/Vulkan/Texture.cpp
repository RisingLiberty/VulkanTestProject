#include "Texture.h"

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"

#include "../Help/HelperMethods.h"

#include <algorithm>

#include <stb/stb_image.h>

namespace
{
	const std::string TEXTURE_PATH = "../data/textures/chalet.jpg";
}

Texture::Texture(LogicalDevice* pCpu, PhysicalDevice* pGpu, CommandPool* pCommandPool):
	m_pCpu(pCpu)
{
	int texWidth, texHeight, texChannels;

	//the stbi_load function takes the file path and number of channels as arguments.
	//the STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn't have one,
	//which is nice for consistency with other textures in the future.
	//The middle 3 parameters are outputs for the dith, height and actual number of channels in the image.
	//The pointer that is returned is the first elemtn in an array of pixel values.
	//The pixels are laid out row by row with 4 bytes per pixel in the case of STBI_rgba_alpha for a total of texWidth * texHeight * 4 values.
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4; //4 -> rgba

	//In Vulkan each of the mip images is stored in different mip levels of a VkImage.
	//Mip level 0 is the original image, and the mip levels after level 0 are commonly referred to as the mip chain.
	//The number of mip levels is specified when the VKImage is created.

	//This calculates the number of levels in the mip chain.
	//The max functions selects the largest dimension. 
	//The log2 function calculates how many times that dimension can be divided by 2.
	//The floor functoin handles case where the largest dimension is not a power of 2.
	//1 is added so that the original image has a mip level.
	m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//The buffer should be in host visible memory so that we can map it
	//and it should be usable as a transfer source so that we can copy it to an image later on.
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pCpu, pGpu);

	void* data;
	vkMapMemory(pCpu->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(pCpu->GetDevice(), stagingBufferMemory);

	//clean up the original pixel array
	stbi_image_free(pixels);

		//We must inform Vulkan that we intend to use the texture image as both the source and destination of a transfer.
		CreateImage(texWidth, texHeight, m_MipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Texture, m_Memory, pCpu, pGpu);

	//Copy the staging buffer to the texture image with 2 steps:
	//Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	//Execute the buffer to image copy operation

	//The image was created with the VK_IMAGE_LAYOUT_UNDEFFINED layout, so that one should be specified as old layout 
	//when transitioning textureImage.
	TransitionImageLayout(m_Texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels, pCommandPool, pCpu);
	CopyBufferToImage(stagingBuffer, m_Texture, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), pCommandPool->GetPool(), pCpu);

	GenerateMipMaps(m_Texture, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, m_MipLevels, pGpu, pCommandPool);
	//To be able to start sampling from the texture image in the shader, we need one last transition
	//to prepare it for shader access.
	//TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);
	vkDestroyBuffer(pCpu->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pCpu->GetDevice(), stagingBufferMemory, nullptr);

	//The code for this function can be based directly on CreateImageViews.
	//The only 2 changes you have to make are the format and the image
	m_TextureView = CreateImageView(m_Texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels, pCpu);

}

Texture::~Texture()
{
	vkDestroyImageView(m_pCpu->GetDevice(), m_TextureView, nullptr);
	vkDestroyImage(m_pCpu->GetDevice(), m_Texture, nullptr);
	vkFreeMemory(m_pCpu->GetDevice(), m_Memory, nullptr);

}

void Texture::GenerateMipMaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, PhysicalDevice* pGpu, CommandPool* pCommandPool)
{
	//Check if image formt supports linear blitting
	//The VkFormatProperties has 3 fields named linearTilingFeatures, optimalTilingFeatures and bufferFeatues.
	//that each describe how the format can be used depending on the way it is used. we create a texture image
	//with the optimal tiling format, so we need to check optimalTilingFeatures. //Support for the linear filtering feature
	//can bechecked with the VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT.
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(pGpu->GetDevice(), format, &formatProps);

	//There are 2 alternatives in this case. You could implement a function that searches common texture image
	//formats for one that does support linear blitting, or you could implement the mipmap generation in software
	//with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way 
	//that you loaded the orignal image.

	//It should be noted that it is uncommon in practice to generate the mipmap levels at runtime anywah.
	//Usually they are pregenerated and stored in the texture file alongside the base level to improve loading speed.
	//Implmenting resizing in software and loading multiple levels from a file is left as an exercise to the reader.
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("texture image format does not support linear blitting!");

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(pCommandPool->GetPool(), m_pCpu);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; ++i)
	{
		//First, we transition level i - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
		//THis transition will wait for level i - 1 to be filled, either from the previous blit command
		//or from vkCmdCopyBufferToImage. The current blit command will wait on this transition
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		//Next, we specify the regoins that will be used in the blit operation.
		//The source mip level is i - 1 and the destination mip level is i.
		//The 2 elements of the srcOffsets array determine the 3D region that data will be blitted from
		//dstOffsets determines the region that data will be blitted to.
		//The x and y dimensions of the dstOffsets[1] are divided by 2 since each mip level is half the size of the previous level.
		//The z dimension of srcOffsets[1] and dstOffsets[1] must be 1, since a 2D image has a depth of 1.
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0,0,0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? static_cast<int32_t>(mipWidth * 0.5f) : 1, mipHeight > 1 ? static_cast<int32_t>(mipHeight * 0.5f) : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		//Now we record the blit command. Note that m_TextureImage is used for both srcImage and dstImage parameter.
		//This is because we're blitting between different levels of the same image.
		//The source mip level was just transitioned to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL and
		//the destination level is still in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL from CreateTextureImage.
		//The last parameter allows us to specify a VkFilter to use in the blit. we have the same filtering options
		//here that we had when making the VkSampler. We use the VK_FILTER_LINEAR to enable interpolation.
		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//This barrier transitions mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
		//This transition waits on the current blit command to finish. All sampling operations will wait on this transiton to finish.
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	//before we end the command buffer, we insert one more pipeline barrier. This barrier transitions the last
	//mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. 
	//This wasn't handled by the loop, since the last mip level is never blitted from
	barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(commandBuffer, pCommandPool->GetPool(), m_pCpu);
}