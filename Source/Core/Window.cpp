//GLFW Defines and includes

#include "Window.h"

Window::Window(int width, int height, const std::string& title, bool isResizable):
	m_Width(width),
	m_Height(height)
{
	glfwInit();

	//Hint that we're not making the window for opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//Hint that the window can't be resized
	glfwWindowHint(GLFW_RESIZABLE, isResizable);

	m_pWindow = std::unique_ptr<GLFWwindow, std::function<void(GLFWwindow*)>>(MakeWindow(width, height, title),
		[](GLFWwindow* pWindow)
	{
		glfwDestroyWindow(pWindow);
	});

	glfwSetWindowUserPointer(m_pWindow.get(), this);
	glfwSetFramebufferSizeCallback(m_pWindow.get(), OnResizeStatic);
}

Window::~Window()
{
}

//The reason that we're creating a static function as a callback is because GLFW does not know how
//to properly call a member function with the right this pointer to our HelloTriangleApplication instance.
//However, we do get a reference to the GLFWwindow in the callback and there is another GLFW function that
//allows you to store an arbitrary pointer inside of it: glfwSetWindowUserPointer
void Window::OnResizeStatic(GLFWwindow* pWindow, int width, int height)
{
	Window* pMyWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(pWindow));
	pMyWindow->OnResize(width, height);
}

void Window::OnResize(int width, int height)
{
	m_Width = width;
	m_Height = height;
}

GLFWwindow* Window::MakeWindow(int width, int height, const std::string& title) const
{
	GLFWwindow* pWindow = glfwCreateWindow(m_Width, m_Height, title.c_str(), nullptr, nullptr);

	if (!pWindow)
		throw std::runtime_error("failed to create window!");

	return pWindow;
}