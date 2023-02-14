#pragma once

#include <string>
#include <inttypes.h>

namespace Photoxel
{
	class Image
	{
	public:
		Image(const std::string& path);
		~Image();

		void Bind();
		void Unbind();

		const void* GetData() const;
		const uint32_t GetWidth() const;
		const uint32_t GetHeight() const;

		void* GetTextureID() const;
	private:
		uint32_t m_Width, m_Height, m_DataFormat;
		uint32_t m_TextureID;
		const void* m_Data;
	};
}