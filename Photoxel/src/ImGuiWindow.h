#pragma once

#include <imgui.h>
#include <string>

namespace Photoxel
{
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& name, bool running, ImGuiWindowFlags flags = 0);
		void Render();
		virtual void Content();
	private:
		std::string m_Name;
		bool m_Closable;
		bool m_Running = true;
		ImGuiWindowFlags m_WindowFlags;
	};
}