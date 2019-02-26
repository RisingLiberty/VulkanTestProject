#include "Vertex.h"

VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
	//A vertex binding describes at which rate to load data from memory throught the vertices.
	//It specifies the number of bytes between data entries and wheter to move to the next data entry
	//after each vertex or after each instance.
	VkVertexInputBindingDescription bindingDescription = {};

	//all of our per-vertex data is packed together in one array, so we're only going to have one binding.
	//the binding paramter specifies the index of the binding in the array of bindings.
	//The stride parameters specifies the number of bytes from one entry to the next
	//the inputRate parameter can have one of the following values:
	//VK_VERTEX_INPUT_RATE_VERTEX: move to the next data entry after each vertex
	//VK_VERTEX_INPUT_RATE_INSTANCE: move to the next data entry after each instance.
	//we're not going to use instanced rendering, so we'll stick to per-vertex data.
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::GetAttributeDescriptions()
{
	//The second structure that describes how to handle vertex input is VkVertexInputAttributeDescription.
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

	//An attribute description struct describes how to extract a vertex attribute from a chuknk of vertex data.
	//originating from a binding description.
	//We have 2 attributes, position and color, so we need 2 attribute description structs

	//POSITION
	//------------------

	//Tells Vulkan from which binding the per-vertex data comes
	attributeDescriptions[0].binding = 0;
	//The location parameter references the location directive of the input in the vertex shader
	//The input in the vertex shader with location 0 is the position, which has 2 32-bit float components
	attributeDescriptions[0].location = 0;
	//The format parameter describes the type of data for the attribute.
	//A bit confusingly, the formata are specified using the same enumeration as color formats.
	//The following shader types and formats are commonly used together:
	//float: VK_FORMAT_R32_SFLOAT
	//vec2: VK_FORMAT_R32G32_SFLOAT
	//vec3: VK_FORMAT_R32G32B32_SFLOAT
	//vec4: VK_FORMAT_R32G32B32A32_SFLOAT
	//The color type (SFLOAT_, UINT, SINT) and bit width should also match the type of the shader input.
	//Examples:
	//ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
	//uvec4: VK_FORMAT_R32G32B32A32, a 4-component vector of 32-bit unsigned integers
	//double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	//specifies the number of bytes since the start of the per-vertex data to read from.
	//The bniding is loading one Vertex at a time and the position attribute(Position) i at an offset of 0 bytes
	//from the beginning of this struct. This is automatically calculated using the offsetof macro.
	attributeDescriptions[0].offset = offsetof(Vertex, Position);

	//COLOR
	//-----------------
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, Color);

	//TEXCOORD
	//-----------------
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

	return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const
{
	return Position == other.Position && Color == other.Color && TexCoord == other.TexCoord;
}