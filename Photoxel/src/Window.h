#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <inttypes.h>
#include <functional>
#include <glm/glm.hpp>

namespace Photoxel
{
	using WindowCloseEventFn = std::function<void()>;

	class Window
	{
	public:
		Window();
		Window(uint32_t width, uint32_t height, const std::string& title);
		virtual ~Window();

		void Update();

		void* GetNativeHandler() const;

		glm::vec2 GetFramebufferSize() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
		void SetIcons(const std::string& filePath);

		int GetKey(int key);

		void SetWindowCloseCallback(WindowCloseEventFn callback);
	private:
		GLFWwindow* m_Window;
		uint32_t m_Width, m_Height;
		std::string m_Title;
		WindowCloseEventFn m_WindowCloseEventFn = []() {};
		bool InitWindow();

		static void WindowCloseEventHandler(GLFWwindow* window);
	};
}
