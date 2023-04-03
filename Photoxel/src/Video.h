#pragma once

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/avutil.h>
}
#include <string>
#include <chrono>

class Video
{
public:
    Video(const std::string& filepath);
    ~Video();
    uint8_t* Read();
    void Free();

    void Pause();
    void Resume();

    void Seek(int second);

    bool IsPaused() const { return m_Paused; }
    int GetWidth() const;
    int GetHeight() const;
    int GetDuration() const;
    int GetCurrentSecond() const;
    uint8_t* GetBuffer() const;
private:
    std::string m_Filename;
    AVFormatContext* m_FormatContext;
    const AVCodec* m_Codec;
    AVCodecContext* m_CodecContext;
    AVFrame* m_Frame;
    AVPacket* m_Packet;
    int m_StreamIndex = -1;
    int m_Width;
    int m_Height;
    AVRational m_Timebase;
    int m_Numerator;
    int m_Denominator;
    double m_Duration;
    double m_CurrentTime;
    int m_LastFramePts;
    uint8_t* m_Buffer = nullptr;
    bool m_FirstTime = true;
    bool m_Paused = false;
    bool m_WasSeek = false;
    std::chrono::steady_clock::time_point m_Start, m_End;
    std::chrono::milliseconds m_ElapsedTime = std::chrono::milliseconds(0);
};
