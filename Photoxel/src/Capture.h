#pragma once
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace Photoxel
{
	class Capture
	{
	public:
		Capture();
		~Capture();
		std::vector<const char*> GetCaptureDeviceNames() const;

		bool StartCapture(int index);
		bool StopCapture();

		int GetWidth();
		int GetHeight();
		uint8_t* GetBuffer();
		bool ReadCapture();
	private:
		static constexpr int MAX_FRAME_SIZE = 1920 * 1080 * 3;
		int m_CaptureDevicesCount;
		std::vector<std::string> m_CaptureDevicesNames;
		std::vector<const char*> m_CaptureDevicesNamesRef;
		int m_StreamIndex = -1;

		std::vector<uint8_t> m_Buffer;
		uint32_t m_Width = 0, m_Height = 0;

		AVFormatContext* m_FormatContext = nullptr;
		AVCodecContext* m_CodecContext = nullptr;
		AVFrame* m_Frame = nullptr;
		AVPacket* m_Packet = nullptr;

		std::thread m_CaptureThread;
		std::atomic<bool> m_CaptureStarted;
		std::mutex m_Mutex;
		SwsContext* m_SwsContext = nullptr;
	};
}