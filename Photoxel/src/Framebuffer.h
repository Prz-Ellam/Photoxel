#pragma once

#include <inttypes.h>
#include <vector>

namespace Photoxel
{
	class Framebuffer
	{
	public:
		Framebuffer(uint32_t width, uint32_t height);
		~Framebuffer();

		virtual void Begin() const;
		virtual void End() const;

		virtual void Resize(uint32_t width, uint32_t height);

		virtual uint32_t GetColorAttachment() const;
		void ClearAttachment(int value = -1) const;
		int ReadPixel(int x, int y) const;

		std::vector<uint8_t> GetData() const;

		uint32_t GetEntityAttachment() const;

		uint32_t GetWidth() const;
		uint32_t GetHeight() const;
	private:
		void CreateAttachment(uint32_t width, uint32_t height);
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;
		uint32_t m_ColorAttachmentID, m_EntityAttachmentID;
	};
}