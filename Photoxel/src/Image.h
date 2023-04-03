#pragma once

#include <string>
#include <inttypes.h>

namespace Photoxel
{
	class Image
	{
	public:
		Image(const std::string& path);
		Image(uint32_t width, uint32_t height, const void* data);
		~Image();

		void Bind();
		void Unbind();

		const char* GetFilename() const;
		const void* GetData() const;
		const void* GetData2() const;
		const uint32_t GetWidth() const;
		const uint32_t GetHeight() const;

		void SetData(uint32_t width, uint32_t height, const void* data);
		void SetData2(uint32_t width, uint32_t height, const void* data);

		void* GetTextureID() const;
	private:
		uint32_t m_Width, m_Height, m_DataFormat;
		uint32_t m_TextureID;
		const void* m_Data;
		std::string m_Filename;
	};
}