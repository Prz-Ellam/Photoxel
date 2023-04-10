#include "Framebuffer.h"
#include <glad/glad.h>
#include <vector>
#include <algorithm>
#include <iostream>

namespace Photoxel
{
	Framebuffer::Framebuffer(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
		glGenFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		CreateAttachment(width, height);
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteTextures(1, &m_ColorAttachmentID);
		glDeleteTextures(1, &m_EntityAttachmentID);
		glDeleteFramebuffers(1, &m_RendererID);
	}

	void Framebuffer::Begin() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		//GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		//glDrawBuffers(2, buffers);

		glViewport(0, 0, m_Width, m_Height);
	}

	void Framebuffer::End() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0u);
	}

	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (m_Width == width && m_Height == height)
		{
			return;
		}

		m_Width = width;
		m_Height = height;
		glDeleteTextures(1, &m_ColorAttachmentID);
		glDeleteTextures(1, &m_EntityAttachmentID);
		glDeleteFramebuffers(1, &m_RendererID);

		glGenFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		CreateAttachment(width, height);
	}

	uint32_t Framebuffer::GetColorAttachment() const
	{
		return m_ColorAttachmentID;
	}

	uint32_t Framebuffer::GetEntityAttachment() const
	{
		return m_EntityAttachmentID;
	}

	// TODO: Parametrizable
	void Framebuffer::ClearAttachment(int value) const
	{
		glClearTexImage(m_ColorAttachmentID, 1, GL_RED_INTEGER, GL_INT, &value);
		glClearTexImage(m_EntityAttachmentID, 0, GL_RED_INTEGER, GL_INT, &value);
	}

	int Framebuffer::ReadPixel(int x, int y) const
	{
		int pixelData = -1;
		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, (void*)&pixelData);
		return pixelData;
	}

	std::vector<uint8_t> Framebuffer::GetData() const {
		int width = m_Width;
		int height = m_Height;
		
		std::vector<uint8_t> buffer(width * height * 4);
		glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentID);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

		return buffer;
		

		/*	std::vector<unsigned char> flippedBuffer(width * height * 4);
			for (int y = 0; y < height; ++y) {
				const uint8_t* srcRow = buffer.data() + (height - y - 1) * width * 4;
				unsigned char* dstRow = flippedBuffer.data() + y * width * 4;
				memcpy(dstRow, srcRow, width * 4);
			}*/

	}

	uint32_t Framebuffer::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Framebuffer::GetHeight() const
	{
		return m_Height;
	}

	void Framebuffer::CreateAttachment(uint32_t width, uint32_t height)
	{
		glGenTextures(1, &m_ColorAttachmentID);
		glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		//glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		//glGenerateMipmap(GL_TEXTURE_2D);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentID, 0);


		glGenTextures(1, &m_OtherTextureID);
		glBindTexture(GL_TEXTURE_2D, m_OtherTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	}
}