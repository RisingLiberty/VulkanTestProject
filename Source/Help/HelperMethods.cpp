#include "HelperMethods.h"

#include "../Vulkan/LogicalDevice.h"
#include "../Vulkan/PhysicalDevice.h"

#include <stdexcept>

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, LogicalDevice* pLogicalDevice)
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
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(pLogicalDevice->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture image view!");

	return imageView;
}

void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, LogicalDevice* pLogicalDevice, PhysicalDevice* pGpu)
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


	if (vkCreateBuffer(pLogicalDevice->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(pLogicalDevice->GetDevice(), buffer, &memRequirements);

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
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties, pGpu);

	if (vkAllocateMemory(pLogicalDevice->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate buffer memory!");

	//the fourth parameter is the offset within the region of memory.
	//Since this memory is allocated specifically for this vertex buffer, the offset is simply 0.
	//If the offset is non-zero, then it is required to be divisble by memREquirements.alignment.
	vkBindBufferMemory(pLogicalDevice->GetDevice(), buffer, bufferMemory, 0);
}

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, PhysicalDevice* pGpu)
{
	const VkPhysicalDeviceMemoryProperties memProperties = pGpu->GetDesc().MemProperties;

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

VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, PhysicalDevice* pGpu)
{
	//The support of a format depends on the tiling mode and usage, so we must also include these as parameters.
	//The support of a format can be queried using the vkGetPhysicalDeviceFormatPropererties function
	for (VkFormat format : candidates)
	{
		//VkFormatProperties contains 3 fields:
		//linearTilingFeatures: Use cases that are supported with linear tiling
		//optimalTilingFeatures: USe cases that are supported with optimal tiling
		//bufferFeatures: Use cases that are supported for buffers.
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(pGpu->GetDevice(), format, &props);

		//Only the first 2 fields are relevant here, and the one we check depends on the tiling parameter
		//of the function
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;

		//If none of the candidate formats support the desired usage, then we can either return a special
		//value of simply throw an exception
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat FindDepthFormat(PhysicalDevice* pGpu)
{
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, pGpu);
}

void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const VkCommandPool& commandPool, LogicalDevice* pCpu)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool, pCpu);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; //Optional
	copyRegion.dstOffset = 0; //Optional
	copyRegion.size = size;

	//Contents of buffers are transferred using the vkCmdCopyBuffer command.
	//It takes the source and destination buffers as arguemtns, and an array of regions to copy.
	//The regions are defined in VkBufferCopy structs and consist of a soruce buffer offset, 
	//destinaiton buffer offset and size. //It is not possible to specify VK_WHOLE_SIZE here, unlike the vkMapMemory command.
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer, commandPool, pCpu);
}


VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool& commandPool, LogicalDevice* pCpu)
{
	//Memory transfer operations are executed using command buffers, just like drawing commands.
	//Therefore we must first allocate a temporary command buffer.
	//You may wish to create a separate command pool for these kinds of short-lived buffers, 
	//because the implementation may be able to apply memory allocation optimizations.
	//You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool genreration in that case.
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(pCpu->GetDevice(), &allocInfo, &commandBuffer);

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

void EndSingleTimeCommands(VkCommandBuffer commandBuffer, const VkCommandPool& commandPool, LogicalDevice* pCpu)
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
	vkQueueSubmit(pCpu->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(pCpu->GetGraphicsQueue());

	vkFreeCommandBuffers(pCpu->GetDevice(), commandPool, 1, &commandBuffer);
}

void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, LogicalDevice* pCpu, PhysicalDevice* pGpu)
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
	imageInfo.samples = numSamples;
	imageInfo.flags = 0; //Optional

	imageInfo.mipLevels = mipLevels;

	if (vkCreateImage(pCpu->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("failed to create image!");

	//Same as buffer allocation except use vkGetImageMemoryRequirements and vkBindImageMemory
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(pCpu->GetDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties, pGpu);

	if (vkAllocateMemory(pCpu->GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate image memory!");

	vkBindImageMemory(pCpu->GetDevice(), image, imageMemory, 0);
}

void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const VkCommandPool& commandPool, LogicalDevice* pCpu)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool, pCpu);

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

	EndSingleTimeCommands(commandBuffer, commandPool, pCpu);
}

bool HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}