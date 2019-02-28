#pragma once

#include <memory>
#include <vector>
#include <string>

#include "../Vulkan/Vertex.h"
#include "../Vulkan/Semaphore.h"
#include "../Vulkan/Fence.h"


//GLFW Defines and includes
#ifndef	GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class Window;
class VulkanInstance;
class Surface;
class PhysicalDevice;
class LogicalDevice;
class SwapChain;
class RenderPass;
class DescriptorSetLayout;
class CommandPool;
class Buffer2D;
class Texture;
class VertexBuffer;
class IndexBuffer;
class DescriptorPool;
class TextureSampler;
class GraphicsPipeline;
class Semaphore;
class Fence;
class DepthBuffer;

class HelloTriangleApplication
{
public:
	HelloTriangleApplication();
	~HelloTriangleApplication();
	void Run();

private:
	void InitializeVulkan();
	void MainLoop();
	void Cleanup();
	   
	std::vector<VkPhysicalDevice> FindGpus();

	void PickPhysicalDevice();
	void DrawFrame();

	//Create semaphores and fences
	void CreateSyncObjects();
	void RecreateSwapChain();
	
private:
	std::unique_ptr<Window> m_UniqueWindow;
	std::unique_ptr<VulkanInstance> m_UniqueInstance;
	std::unique_ptr<Surface> m_UniqueSurface;
	std::unique_ptr<PhysicalDevice> m_UniqueGpu;
	std::unique_ptr<LogicalDevice> m_UniqueCpu;
	std::unique_ptr<SwapChain> m_UniqueSwapChain;
	std::unique_ptr<RenderPass> m_UniqueRenderPass;
	std::unique_ptr<DescriptorSetLayout> m_UniqueDescriptorSetLayout;
	std::unique_ptr<GraphicsPipeline> m_UniquePipeline;
	std::unique_ptr<CommandPool> m_UniqueCommandPool;
	std::unique_ptr<TextureSampler> m_UniqueSampler;

	//In MSAA, each pixel is sampled in an offscreen buffer which is then rendered to the screen.
	//This new buffer is silghtly different from regular images we've been rendering to, 
	//they have to be able to store more than one sample per pixel. Once a multisampled buffer is created,
	// it has to be resolved to the default framebuffer (which stores only a single sample per pixel). 
	//This is why we have to create an additional render target and modifgy our current drawing procsess.
	//We only need one render target since only one drawing operation is actie at a time, just like with
	//the depth buffer.
	std::unique_ptr<Buffer2D> m_UniqueRenderTarget;
	std::unique_ptr<DepthBuffer> m_UniqueDepthBuffer;
	std::unique_ptr<Texture> m_UniqueTexture;
	std::unique_ptr<VertexBuffer> m_UniqueVertexBuffer;
	std::unique_ptr<IndexBuffer> m_UniqueIndexBuffer;
	std::unique_ptr<DescriptorPool> m_UniqueDescriptorPool;

	//Each frame should have its own set of semaphores
	std::vector<std::unique_ptr<Semaphore>> m_ImageAvailableSemaphores;
	std::vector<std::unique_ptr<Semaphore>> m_RenderFinishedSemaphores;
	std::vector<std::unique_ptr<Fence>> m_InFlightFences;

	size_t m_CurrentFrame = 0;
	bool m_FrameBufferResized = false;

	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const int MAX_FRAMES_IN_FLIGHT = 2;

	//Interleaving vertex attributes: all vertices and their attributes are defined in 1 buffer
	std::vector<Vertex> m_Vertices;

	//It is possible to use either uin16_t or uin32_t for your index buffer depending on the number
	//of entries in vertices. We can stick to uint16_t for now because we're using less than 65534 unique vertices
	std::vector<uint32_t> m_Indices;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	const std::string MODEL_PATH = "../data/meshes/chalet.obj";

};