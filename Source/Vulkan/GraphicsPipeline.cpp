#include "GraphicsPipeline.h"

#include "LogicalDevice.h"
#include "Vertex.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "DescriptorSetLayout.h"
#include "PipelineLayout.h"

#include "../Help/HelperMethods.h"

#include "ShaderModule.h"

GraphicsPipeline::GraphicsPipeline(LogicalDevice* pCpu, SwapChain* pSwapChain, RenderPass* pRenderPass, DescriptorSetLayout* pDescSetLayout) :
	m_pCpu(pCpu)
{
	std::vector<char> vertShaderCode = ReadFile("../data/shaders/bin/vert.spv");
	std::vector<char> fragShaderCode = ReadFile("../data/shaders/bin/frag.spv");

	ShaderModule vertShader = ShaderModule(pCpu, vertShaderCode);
	ShaderModule fragShader = ShaderModule(pCpu, fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	//the next 2 members specify the shader module containing the code, and the function to invoke.
	//that means that it's possible to combine multiple fragment shaders into a single shader module and
	//and use different entry points to differentiate between their behaviors. In this case we'll stick to the standard main.

	//However there is one more (optional) member, pSpecializationInfo, which we won't be using here, but is worth discussing.
	//It allows you to specify values for shader constants. You can use a single shader module where its behavior
	//can be used in it. This is more efficient than configuring the shader using variables at render time,
	//because the compiler can do optimizations like eliminating if statemetns that depend on these values.
	//If you don't have any constans like that, then you can set the member to nullptr, which our struct initialization does automatically.
	vertShaderStageInfo.module = vertShader.GetModule();
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShader.GetModule();
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::GetAttributeDescriptions();

	//The pVertexBindingDescriptions and pVertexAttributeDescriptions members point to an array
	//of structs that describe that aformentinoed details for loading vertex data.
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; //Optional
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); //Optional

	//Normally, the vertices are loaded from the vertex buffer by index in sequential order,
	//but with an element buffer you can specify the indices to use yourself.
	//This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to VK_TRUE,
	//then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)pSwapChain->GetExtent().width;
	viewport.height = (float)pSwapChain->GetExtent().height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = pSwapChain->GetExtent();

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns
	//it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test,
	//and it can be configured to output fragments that fill entire polygons or just the edges(wireframe rendering).
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	//if depthClampEnable is set to VK_TRUE, the fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
	//this is useful in some special cases like shadow maps. using this requires enabling a GPU feature.
	rasterizer.depthClampEnable = VK_FALSE;

	//if rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the rasterizer stage.
	//this basically disables any output to the framebuffer.
	rasterizer.rasterizerDiscardEnable = VK_FALSE;

	//the polygonMode determines how fragments are generated for geometry. the following modes are available:
	//VK_POLYGON_MODE_FILL: fill the area of the polygon fragments
	//VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
	//VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	//the linewidth member is straightforward, it describes the thickness of lines in terms of number of fragments.
	//the maximum line widht that is supported depends on the hardware and any line thicker than 1.0f requires you to enable
	//the wideLines GPU feature
	rasterizer.lineWidth = 1.0f;

	//the cullMode variable determines the type of face culling to use.
	//you can disbale culling, cull the front faces, cull the back faces or both.
	//The frontFace variable specifies the vertex order for faces to be considered front-facing and can be clockwise or counterclockwise.
	//Because we scale the Y scale axis by -1 we need to draw in counter clockwise order.
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	//the rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment's lsope.
	//this is sometimes used for shadow mapping, but we won't be using it. just set depthBiasEnable to VK_FALSE
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; //Optional
	rasterizer.depthBiasClamp = 0.0f; //Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; //Optional

	VkPipelineMultisampleStateCreateInfo multiSampling = {};
	multiSampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampling.sampleShadingEnable = VK_TRUE; //Enable sanple shading in the pipeline
	multiSampling.rasterizationSamples = pRenderPass->GetSamplesCount();
	multiSampling.minSampleShading = 0.2f; //Optional min fraction for sample shader: closer to one is smoother
	multiSampling.pSampleMask = nullptr; //Optional
	multiSampling.alphaToCoverageEnable = VK_FALSE; //Optional
	multiSampling.alphaToOneEnable = VK_FALSE; //Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachement = {};
	//There are two types of structs to configure color blending.
	//The first struct, VkPipelineColorBlendAttachementState contains the configuration per attached framebuffer.
	//The second struct, VkPipelineColorBlendStateCreateInfo contains the global color blending settings.
	colorBlendAttachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;;
	colorBlendAttachement.blendEnable = VK_FALSE;
	colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
	colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD; //Optional
	colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //Optional
	colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //Optional
	colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD; //Optional

	//This per-framebuffer struct allows you to configure the first way of color blending.
	//the operations that will be performed are best demonstrated using the following pseudocode

	/*
	if (blendEnable)
	{
		finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
		finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
	}

	else
		finalColor = newColor;
	*/

	//if blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified.
	//Otherwise, the two mixing operations are performed to compute a new color.
	//The resulting color AND'd with the colorWriteMask to determine which channels are actually passed through

	//The most common way to use color blending is to implement alpha blending, where we want the new color to be blended
	//with the old color based on its opacity. the finalColor should then be computed as follows.

	/*
	finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
	finalColor.a = newAlpha.a;
	*/

	//this can be accomplished with the following parameters
	/*
	colorBlendAttachement.blendEnable = VK_TRUE;
	colorBlendAttachement.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachement.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachement.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachement.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachement.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachement.alphaBlendOp = VK_BLEND_OP_ADD;
	*/

	//the second structure references the array of structures for all of the framebuffers and allows you to 
	//set blend constants that you can use as blend factors in the aformentioned calculations.

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; //Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachement;
	colorBlending.blendConstants[0] = 0.0f; //Optional
	colorBlending.blendConstants[1] = 0.0f; //Optional
	colorBlending.blendConstants[2] = 0.0f; //Optional
	colorBlending.blendConstants[3] = 0.0f; //Optional

	//If you want to use the second method of blending(bitwise combination), then
	//you should set logicOpEnable to VK_TRUE. the bitwise operation can then be specified in the logicOp field.
	//Note that this will automatically disable the first method, as if you had set blendenable to VK_FALSE for every
	//attached framebuffer! the colorWriteMask will also be used in this mode to determine which channels in the framebuffer
	//will actually be affected. it is also possible to disable both modes, as we've done here, in which case the fragment colors
	//will be written to the framebuffer unmodified.

	//A limited amount of the state that we've specified in the previous structs can actually be changed without
	//recreating the pipeline. Examples are the size of the viewport, line width and blend constants.
	//Examples are the size of the viewport, line width and blend constant. if you want to do that, then yo'll
	//have to fill in a VkPipelineDynamicStateCreateInfo structure like this:
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	////We need to specifiy the descriptor set layout during pipeline creation to tell Vulkan which descriptors
	////the shaders will be using. Descriptor set layouts are specified in the pipeline layout objects.
	////Modify the VkPipelineLayoutCreateInfo to reference the layout object.
	//VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	//pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	//pipelineLayoutInfo.setLayoutCount = 1; //Optional
	//pipelineLayoutInfo.pSetLayouts = &m_UniqueDescriptorSetLayout->GetDescriptorSetLayout(); //Optional
	//pipelineLayoutInfo.pushConstantRangeCount = 0; //Optional
	//pipelineLayoutInfo.pPushConstantRanges = nullptr; //Optional

	//if (vkCreatePipelineLayout(m_UniqueCpu->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	//	throw std::runtime_error("failed to create pipeline layout");
	m_UniqueLayout = std::make_unique<PipelineLayout>(pCpu, pDescSetLayout);

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	//The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer
	//to see if they should be discarded. 
	depthStencil.depthTestEnable = VK_TRUE;

	//The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually
	//be written to the depth buffer. This is useful for drawing transparent objects.
	//They should be copmared to the previously rendered opaque objects, but not cause further away transparent objects 
	//to not be drawn.
	depthStencil.depthWriteEnable = VK_TRUE;

	//The depthCompareOp field specifies the copmarison that is performed to keep or discard fragments.
	//We're stikcing to the convention of lower depth = closer, so the depth  of new fragments should be less.
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	//The depthBoundsTestEnable minDepthBounds and maxDepthBounds fields are used for the optional depth bound test.
	//Basically, this allows you to only keep fragments that fall within the specified depth range.
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; //Optional
	depthStencil.maxDepthBounds = 1.0f; //Optional

	//The last 3 fields configure the stencil buffer operations, which we alse won't be using in this tutorial
	//If you want to use these operations, then you will have to make sure that the format of the depth/stencil image
	//contains a stencil components
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; //Optional
	depthStencil.back = {}; //Optional		

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	//Start by referencing the array of vkPipelineShaderStageCreateInfo structs
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	//Reference all of the strucutures describing the fixed function stage
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multiSampling;
	pipelineInfo.pDepthStencilState = &depthStencil; //Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; //Optional

	//then the pipeline layout, which is a Vulkan handle rather than a struct pointer
	pipelineInfo.layout = m_UniqueLayout->GetPipelineLayout();

	//finally the renderpass and the index of the sub pass where this graphics pipeline will be used
	//It is also possible to use other render passes with this pipeline instead of this specific instance,
	//but they have to be compatible with renderPass.
	//The requirements for caompatibilty are described here <https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility>, but we wo'nt be using that feauture
	pipelineInfo.renderPass = pRenderPass->GetRenderPass();
	pipelineInfo.subpass = 0;

	//there are actually 2 more parameters, basePipelineHandle and basePielineIndex.
	//Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline.
	//The idea of pipeline derivatives is that is is less expesnsive to set up pipelines when they have much
	//functionality in common with an existing pipeline and switching between pipelines from the same parent
	//pipleline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
	//right now there is only a single pipeline, so we'll simply specify a null handle and an invalid index.
	//These value are only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags fields of vkGraphicsPipelinecreateInfo
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //Optional
	pipelineInfo.basePipelineIndex = -1; //Optional

	//The vkCreateGraphicsPipelines function actually has more parameters than the usual object creation functions in Vulkan.
	//It is designed to take multiple vkGraphicsPipelineCreateInfo objects and create multiple VkPipeline objects in a single call.
	//The second parameter for which we've passed VK_NULL_HANDLE argument, references an optional VkPipelineCache object.
	//A pipeline cache can be used to store and reuse data relevant to pipeline creation across multiple calls to vkCreateGraphicsPipelines.
	//and even across program executions if the cache is stored to a file. This makes it possible to significantly speed up pipeline
	//creation at a later time.
	if (vkCreateGraphicsPipelines(pCpu->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipeline!");

}

GraphicsPipeline::~GraphicsPipeline()
{

}

