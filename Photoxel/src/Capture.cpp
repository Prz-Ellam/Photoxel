#include "Capture.h"
#include "escapi.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
}

namespace Photoxel
{
	Capture::Capture()
	{
		m_CaptureDevicesCount = setupESCAPI();
		if (m_CaptureDevicesCount == 0)
		{
			printf("ESCAPI initialization failure or no devices found.\n");
			return;
		}

		for (int i = 0; i < m_CaptureDevicesCount; i++)
		{
			char cameraname[255];
			getCaptureDeviceName(i, cameraname, 255);
			m_CaptureDevicesNames.push_back(cameraname);
			m_CaptureDevicesNamesRef.push_back(m_CaptureDevicesNames[i].c_str());
		}

        avdevice_register_all();
	}

	std::vector<const char*> Capture::GetCaptureDeviceNames() const
	{
		return m_CaptureDevicesNamesRef;
	}

	bool Capture::StartCapture(int captureIndex)
	{
        StopCapture();

        const AVInputFormat* inputFormat = av_find_input_format("dshow");
        m_FormatContext = avformat_alloc_context();
        std::string name = "video=" + m_CaptureDevicesNames[captureIndex];
        int result = avformat_open_input(&m_FormatContext, name.c_str(), inputFormat, nullptr);
        if (result < 0) {
            return false;
        }

        result = avformat_find_stream_info(m_FormatContext, nullptr);
        if (result < 0) {
            return false;
        }

        m_StreamIndex = av_find_best_stream(m_FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (m_StreamIndex < 0) {
            return false;
        }

        AVStream* stream = m_FormatContext->streams[m_StreamIndex];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder) {
            return false;
        }

        m_CodecContext = avcodec_alloc_context3(decoder);
        if (!m_CodecContext) {
            return false;
        }

        result = avcodec_parameters_to_context(m_CodecContext, stream->codecpar);
        if (result < 0) {
            return false;
        }

        result = avcodec_open2(m_CodecContext, decoder, nullptr);
        if (result < 0) {
            return false;
        }

        m_Frame = av_frame_alloc();
        if (!m_Frame) {
            return false;
        }

        m_Packet = av_packet_alloc();
        if (!m_Packet) {
            return false;
        }

        return true;
	}

	bool Capture::StopCapture()
	{
        if (m_FormatContext)
        {
            avformat_close_input(&m_FormatContext);
            m_FormatContext = nullptr;
        }

        if (m_CodecContext)
        {
            avcodec_free_context(&m_CodecContext);
            m_CodecContext = nullptr;
        }

        if (m_Frame)
        {
            av_frame_free(&m_Frame);
            m_Frame = nullptr;
        }

        if (m_Packet)
        {
            av_packet_free(&m_Packet);
            m_Packet = nullptr;
        }

        return true;
	}

    int Capture::GetWidth() const
    {
        return m_Frame->width;
    }

    int Capture::GetHeight() const
    {
        return m_Frame->height;
    }

    uint8_t* Capture::GetBuffer() const
    {
        return m_Buffer;
    }

    bool Capture::ReadCapture()
    {
        int result = av_read_frame(m_FormatContext, m_Packet);
        if (result < 0) {
            return false;
        }

        if (m_Packet->stream_index != m_StreamIndex) {
            return false;
        }

        result = avcodec_send_packet(m_CodecContext, m_Packet);
        if (result < 0) {
            return false;
        }

        result = avcodec_receive_frame(m_CodecContext, m_Frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return false;
        }
        else if (result < 0) {
            return false;
        }

        m_Buffer = new uint8_t[m_Frame->width * m_Frame->height * 4];
        SwsContext* swsContext = sws_getContext(m_Frame->width, m_Frame->height,
            m_CodecContext->pix_fmt, m_Frame->width, m_Frame->height,
            AV_PIX_FMT_RGB0, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!swsContext) {
            return false;
        }

        uint8_t* dest[4] = { m_Buffer, nullptr, nullptr, nullptr };
        int stride[4] = { m_Frame->width * 4, 0, 0, 0 };
        sws_scale(swsContext, m_Frame->data, m_Frame->linesize, 0, m_Frame->height, dest, stride);
        sws_freeContext(swsContext);

        return true;
    }
}