#include "TextureSampler.h"

#include "LogicalDevice.h"

TextureSampler::TextureSampler(LogicalDevice* pCpu, uint32_t mipLevels) :
	m_pCpu(pCpu)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	//The magFilter and minFilter fields specify how to interpolate texels that are magnified or minified.
	//Magnification concerns the oversampling problem, minification concerns undersampling.
	//The choices are VK_FILTER_NEAREST and VK_FILTER_LINEAR.
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	//The addressing mode can be specified per axis using the addressMode fields.
	//The available values are: 
	//VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
	//VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repear, but inverts the coordinates to mirror the image when going beyond the dimensions.
	//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
	//VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
	//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Returns a solid color when sampling beyond the dimensions of the image.
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	//These 2 fields specify if anisotropic filtering should be used.
	//There is no reason not use this unless performance is a concern.
	//The maxAnisotropy field limits the amount of texel samples that can be used to calculate the final color.
	//A lower value results in better performance, but lower quality results.
	//There is no graphics hardware available today that will use more than 16 samples, because the difference 
	//is negligible beyond that point.
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	//Disable antisotropic filtering
	//samplerInfo.anisotropyEnable = VK_FALSE;
	//samplerInfo.maxAnisotropy = 1;

	//The borderColor specifies which color is returned when sampling beyond the image with clamp to border addressing mode.
	//it is possible to return black, white, transparent in either float or int formats. You cannot specify an arbitrary color.
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	//The unnormalizedCoordinates filed specifies which coordinate system you want to use to address texels in an image.
	//If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth] and [0, texHeight] range.
	//if is is VK_FALSE, then the texesl are addressed using the [0,1] range on all axes.
	//Real-World applications almost always use normalized coordinates, because then it's possible to use textures of varying
	//resolutions with the exact same coordinates.
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	//if a comparison function is enabled, then texels will first be compared to a value and
	//the result of that comparison is used in filtering operations. This is mainly used for percentage-closer
	//filtering on shadow maps.
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	//All of these fields apply to mipmapping. We will look at mipmapping in a later chapter, 
	//but basically it's another type of filter that can be applied.
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	//Note that the sampler does not reference a VkImage anywhere. The sampler is a distinct object
	//that provides an interface to extract colors from a texture.
	//It can be applied to any image you want, whether it is 1D, 2D or 3D. This is different from
	//many older APIs, which combinded texture images and filtering into a single state.
	if (vkCreateSampler(pCpu->GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
		throw std::runtime_error("failed to create texture sampler!");
}

TextureSampler::~TextureSampler()
{
	vkDestroySampler(m_pCpu->GetDevice(), m_Sampler, nullptr);
}