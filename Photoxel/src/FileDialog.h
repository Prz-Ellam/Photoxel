#pragma once

#include <string>
#include "Window.h"

namespace Photoxel
{
	class FileDialog
	{
	public:
		static std::string OpenFile(const Window& window, const std::string& filter);
		static std::string SaveFile(const Window& window, const std::string& filter);
	private:
	};

}