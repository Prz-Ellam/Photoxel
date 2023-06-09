#include "Capture.h"
#include <mutex>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
}
#include <iostream>

#define SIZE 1024

namespace Photoxel
{
    HRESULT Capture::EnumerateDevices(REFGUID category, IEnumMoniker** ppEnum)
    {
        HRESULT hr;
        ICreateDevEnum* pDevEnum = NULL;
        hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
            IID_ICreateDevEnum, (void**)&pDevEnum);

        if (SUCCEEDED(hr))
        {
            hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
            if (hr == S_FALSE)
            {
                hr = VFW_E_NOT_FOUND;
            }
            pDevEnum->Release();
        }
        return hr;
    }

    void Capture::DisplayDeviceInformation(IEnumMoniker* pEnum)
    {
        IMoniker* pMoniker = NULL;

        while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
        {
            IPropertyBag* pPropBag;
            HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
            if (FAILED(hr))
            {
                pMoniker->Release();
                continue;
            }

            VARIANT var;
            VariantInit(&var);

            hr = pPropBag->Read(L"Description", &var, 0);
            if (FAILED(hr))
            {
                hr = pPropBag->Read(L"FriendlyName", &var, 0);
            }
            if (SUCCEEDED(hr))
            {
                printf("%S\n", var.bstrVal);
                std::wstringstream ss;

                ss << var.bstrVal;
                std::wstring deviceName = ss.str();
                std::string strDeviceName = std::string(deviceName.begin(), deviceName.end());

                m_CaptureDevicesNames.emplace_back(strDeviceName);
                m_CaptureDevicesNamesRef.push_back(strDeviceName.c_str());
                VariantClear(&var);

                ss.clear();
            }

            pPropBag->Release();
            pMoniker->Release();
        }
    }

	Capture::Capture()
	{
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr))
        {
            IEnumMoniker* pEnum;

            hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
            if (SUCCEEDED(hr))
            {
                DisplayDeviceInformation(pEnum);
                pEnum->Release();
            }

            CoUninitialize();
        }

        avdevice_register_all();
        m_Buffer.resize(MAX_FRAME_SIZE);
	}

    Capture::~Capture()
    {
        StopCapture();
    }

	std::vector<const char*> Capture::GetCaptureDeviceNames() const
	{
        std::vector<const char*> charVec;
        for (const auto& str : m_CaptureDevicesNames)
        {
            charVec.push_back(str.c_str());
        }
		return charVec;
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

        m_Width = m_CodecContext->width;
        m_Height = m_CodecContext->height;

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

        m_SwsContext = sws_getContext(m_Width, m_Height, m_CodecContext->pix_fmt, 
            SIZE, SIZE, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!m_SwsContext) {
            return false;
        }

        m_CaptureStarted = true;
        m_CaptureThread = std::thread([&]() {
            while (m_CaptureStarted) {
                ReadCapture();
            }
        });

        return true;
	}

	bool Capture::StopCapture()
	{
        if (m_CaptureThread.joinable()) {
            m_CaptureStarted = false;
            m_CaptureThread.join();
        }

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

        if (m_SwsContext)
        {
            sws_freeContext(m_SwsContext);
            m_SwsContext = nullptr;
        }

        return true;
	}

    int Capture::GetWidth()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Width;
    }

    int Capture::GetHeight()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Height;
    }

    uint8_t* Capture::GetBuffer()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Buffer.data();
    }

    bool Capture::ReadCapture()
    {
        int result = av_read_frame(m_FormatContext, m_Packet);
        if (result < 0) {
            av_packet_unref(m_Packet);
            return false;
        }

        if (m_Packet->stream_index != m_StreamIndex) {
            av_packet_unref(m_Packet);
            return false;
        }

        result = avcodec_send_packet(m_CodecContext, m_Packet);
        if (result < 0) {
            av_packet_unref(m_Packet);
            return false;
        }

        result = avcodec_receive_frame(m_CodecContext, m_Frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            av_packet_unref(m_Packet);
            return false;
        }
        else if (result < 0) {
            av_packet_unref(m_Packet);
            return false;
        }

        uint8_t* dest[4] = { m_Buffer.data(), nullptr, nullptr, nullptr};
        int stride[4] = { SIZE * 3, 0, 0, 0 };
        sws_scale(m_SwsContext, m_Frame->data, m_Frame->linesize, 0, m_Frame->height, dest, stride);

        av_packet_unref(m_Packet);
        av_frame_unref(m_Frame);
        return true;
    }
}