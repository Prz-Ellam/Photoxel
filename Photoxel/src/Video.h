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
#include <vector>
#include <thread>
#include <mutex>

class Video
{
public:
    Video(const std::string& filepath);
    ~Video();
    
    void Pause();
    void Resume();

    uint8_t* GetFrame();

    void Seek(int second);

    bool IsPaused() const { return m_Paused; }
    int GetWidth();
    int GetHeight();
    int GetDuration();
    int GetCurrentSecond();
    int Read();
private:
    std::string m_Filename;
    AVFormatContext* m_FormatContext;
    AVCodecContext* m_CodecContext;
    AVFrame* m_Frame = nullptr;
    AVPacket* m_Packet = nullptr;
    SwsContext* m_SwsContext = nullptr;
    int m_StreamIndex = -1;
    int64_t m_Timestamp = 0;
    int m_Width;
    int m_Height;
    AVRational m_Timebase;
    double m_Duration;
    double m_CurrentTime;
    int m_LastFramePts;
    std::vector<uint8_t> m_Buffer2;
    bool m_FirstTime = true;
    bool m_Paused = false;
    bool m_WasSeek = false;
    bool m_IsSeeking = false;
    std::chrono::steady_clock::time_point m_Start, m_End;
    std::chrono::milliseconds m_ElapsedTime = std::chrono::milliseconds(0);

    std::thread m_CaptureThread;
    std::atomic<bool> m_CaptureStarted;
    std::mutex m_Mutex;
    std::condition_variable m_ConditionVariable;
};
