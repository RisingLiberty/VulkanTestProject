#pragma once

#include <memory>

class Buffer2D;
class LogicalDevice;
class CommandPool;
class RenderPass;
class PhysicalDevice;

class DepthBuffer
{
public:
	DepthBuffer(LogicalDevice* pCpu, CommandPool* pCommandPool, RenderPass* pRenderPass, PhysicalDevice* pGpu, uint32_t width, uint32_t height);
	~DepthBuffer();

	Buffer2D* GetBuffer() const { return m_Buffer.get(); }

private:
	std::unique_ptr<Buffer2D> m_Buffer;
};