#include "FileDialog.h"

#include <Windows.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Application.h"

namespace Photoxel
{
	std::string FileDialog::OpenFile(const Window& window, const std::string& filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window.GetNativeHandler());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrDefExt = "jpg";
		ofn.nFilterIndex = 1;

		std::vector<char> filterData;
		filterData.reserve(filter.size() + 1);
		for (auto& c : filter) {
			filterData.emplace_back(c == '|' ? '\0' : c);
		}
		filterData.emplace_back('\0');

		ofn.lpstrFilter = filterData.data();
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn))
			return ofn.lpstrFile;

		return std::string();
	}

	std::string FileDialog::SaveFile(const Window& window, const std::string& filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { "image.jpg"};
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)window.GetNativeHandler());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = "JPG (.jpg)\0*.jpg\0PNG (.png)\0*.png";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		ofn.lpstrDefExt = strchr(filter.c_str(), '\0') + 1;

		if (GetSaveFileNameA(&ofn))
			return ofn.lpstrFile;

		return std::string();
	}
}