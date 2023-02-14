#include "Window.h"

namespace Photoxel
{
	Window::Window()
		: m_Width(1280), m_Height(720), m_Title("Photoxel Application")
	{
		InitWindow();
	}

	Window::Window(uint32_t width, uint32_t height, const std::string& title)
		: m_Width(width), m_Height(height), m_Title(title)
	{
		InitWindow();
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_Window);
		// Esto solo es cierto si existe una sola ventana
		glfwTerminate();
	}

	bool Window::InitWindow()
	{
		if (!glfwInit())
		{
			return false;
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			return false;
		}

		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSwapInterval(1);

		glfwSetWindowCloseCallback(m_Window, Window::WindowCloseEventHandler);

		return true;
	}

	uint32_t Window::GetHeight() const
	{
		return m_Height;
	}

	void Window::WindowCloseEventHandler(GLFWwindow* window)
	{
		Window *photoxelWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		photoxelWindow->m_WindowCloseEventFn();
	}

	void Window::Update()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	void* Window::GetNativeHandler() const
	{
		return m_Window;
	}

	glm::vec2 Window::GetFramebufferSize() const
	{
		int displayW = 0, displayH = 0;
		glfwGetFramebufferSize((GLFWwindow*)m_Window, &displayW, &displayH);
		return glm::vec2(displayW, displayH);
	}

	uint32_t Window::GetWidth() const
	{
		return m_Width;
	}

	void Window::SetWindowCloseCallback(WindowCloseEventFn callback)
	{
		m_WindowCloseEventFn = callback;
	}
}