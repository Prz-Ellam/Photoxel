#pragma once
#include <vector>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Photoxel
{
	class Capture
	{
	public:
		Capture();
		std::vector<const char*> GetCaptureDeviceNames() const;

		bool StartCapture(int index);
		bool ReadCapture();
		bool StopCapture();

		int GetWidth() const;
		int GetHeight() const;
		uint8_t* GetBuffer() const;
	private:
		int m_CaptureDevicesCount;
		std::vector<std::string> m_CaptureDevicesNames;
		std::vector<const char*> m_CaptureDevicesNamesRef;
		int m_StreamIndex;

		uint8_t* m_Buffer = nullptr;

		AVFormatContext* m_FormatContext = nullptr;
		AVCodecContext* m_CodecContext = nullptr;
		AVFrame* m_Frame = nullptr;
		AVPacket* m_Packet = nullptr;
	};
}