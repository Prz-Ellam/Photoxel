#include "ImGuiWindow.h"
#include <iostream>

namespace Photoxel
{
	ImGuiWindow::ImGuiWindow(const std::string& name, bool closable, ImGuiWindowFlags flags)
		: m_Name(name), m_Closable(closable), m_WindowFlags(flags)
	{
	}

	void ImGuiWindow::Render()
	{
		if (!m_Running) return;
		ImGui::Begin(m_Name.c_str(), m_Closable ? &m_Running : nullptr, m_WindowFlags);
		Content();
		ImGui::End();
	}
	
	void ImGuiWindow::Content()
	{
		ImGui::Button("Hola");
	}
}