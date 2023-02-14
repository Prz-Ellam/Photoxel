#pragma once

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#define IMAPP_IMPL
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <memory>

namespace Photoxel
{
	class Window;

	class ImGuiLayer
	{
	public:
		ImGuiLayer(std::shared_ptr<Window>& window);
		~ImGuiLayer();
		void Begin();
		void End();
		void SetTheme();
		void EnableDockspace();
	private:
		std::shared_ptr<Window> m_Window;
		bool m_DockspaceOpen = true;
		bool m_OptFullscreenPersistant = true;
	};
}