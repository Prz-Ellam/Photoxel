#include "Image.h"
#include <glad/glad.h>
#include "stb_image.h"
#include <filesystem>

namespace Photoxel
{
    Image::Image(const std::string& path)
    {
		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
		m_Data = data;
		m_Width = width;
		m_Height = height;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

	Image::~Image()
	{
		glDeleteTextures(1, &m_TextureID);
	}

	void Image::Bind()
	{
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
	}

	void Image::Unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0u);

	}

	const void* Image::GetData() const
	{
		return m_Data;
	}

	const uint32_t Image::GetWidth() const
	{
		return m_Width;
	}

	const uint32_t Image::GetHeight() const
	{
		return m_Height;
	}

	void* Image::GetTextureID() const
	{
		return (void*)m_TextureID;
	}
}