#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <functional>

class Window
{
public:
	Window(int width, int height, const std::string& title, bool isResizable);
	~Window();

	//window isn't resizable, but this can change
	static void OnResizeStatic(GLFWwindow* pWindow, int width, int height);
	void OnResize(int width, int height);
	GLFWwindow* GetGLFWWindow() const { return m_pWindow.get(); }

private:
	GLFWwindow* MakeWindow(int width, int height, const std::string& title) const;

private:
	std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>> m_pWindow;
	int m_Width;
	int m_Height;
};