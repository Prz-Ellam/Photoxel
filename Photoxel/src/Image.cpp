#include "Image.h"
#include <glad/glad.h>
#include "stb_image.h"
#include <filesystem>

namespace Photoxel
{
	std::string GetImageName(const std::string& filePath)
	{
		// Split the file path string based on the directory separator character
		std::vector<std::string> pathComponents;
		size_t startPos = 0;
		size_t endPos = 0;
		while ((endPos = filePath.find_first_of("/\\", startPos)) != std::string::npos)
		{
			if (endPos > startPos)
			{
				pathComponents.push_back(filePath.substr(startPos, endPos - startPos));
			}
			startPos = endPos + 1;
		}
		if (startPos < filePath.length())
		{
			pathComponents.push_back(filePath.substr(startPos));
		}

		// Return the last component of the path (i.e. the image name)
		if (!pathComponents.empty())
		{
			return pathComponents.back();
		}
		else
		{
			return "";
		}
	}

    Image::Image(const std::string& path)
    {
		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		int width, height, channels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
		m_Data = data;
		m_Width = width;
		m_Height = height;
		m_Filename = GetImageName(path.c_str());

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width >> 1, height >> 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

	Image::Image(uint32_t width, uint32_t height, const void* data)
	{
		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		m_Width = width;
		m_Height = height;
		m_Data = data;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width >> 1, height >> 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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

	const char* Image::GetFilename() const
	{
		return m_Filename.c_str();
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

	std::vector<uint8_t> Image::GetData2(int level)
	{
		std::vector<uint8_t> buffer(512 * 512 * 3);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
		int exW, exH;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &exW);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &exH);
		glGetTexImage(GL_TEXTURE_2D, 1, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
		return buffer;
	}

	void Image::SetData(uint32_t width, uint32_t height, const void* data)
	{
		m_Width = width;
		m_Height = height;
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (m_Width == 1) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (m_Width == 1) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//glGenerateMipmap(GL_TEXTURE_2D);
		//glTexImage2D(GL_TEXTURE_2D, 1, GL_RGB8, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		Unbind();
	}

	void Image::SetData2(uint32_t width, uint32_t height, const void* data)
	{
		m_Width = width;
		m_Height = height;
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (m_Width == 1) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (m_Width == 1) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		Unbind();
	}
}