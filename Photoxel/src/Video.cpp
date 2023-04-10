#include "Video.h"
#include <iostream>
extern "C" {
#include <libavutil/time.h>
}

Video::Video(const std::string& filepath) {
    m_Filename = filepath;

    m_FormatContext = avformat_alloc_context();
    int result = avformat_open_input(&m_FormatContext, m_Filename.c_str(), nullptr, nullptr);
    if (result < 0) {
        return;
    }

    result = avformat_find_stream_info(m_FormatContext, nullptr);
    if (result < 0) {
        return;
    }

    int64_t duration = m_FormatContext->duration;
    m_Duration = static_cast<double>(duration) / AV_TIME_BASE;

    m_StreamIndex = av_find_best_stream(m_FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_StreamIndex < 0) {
        return;
    }

    AVStream* stream = m_FormatContext->streams[m_StreamIndex];
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        return;
    }
    m_Timebase = stream->time_base;

    m_CodecContext = avcodec_alloc_context3(decoder);
    if (!m_CodecContext) {
        return;
    }

    result = avcodec_parameters_to_context(m_CodecContext, stream->codecpar);
    if (result < 0) {
        return;
    }

    m_Width = m_CodecContext->width;
    m_Height = m_CodecContext->height;
    m_Buffer2.resize(m_Width * m_Height * 4);

    result = avcodec_open2(m_CodecContext, decoder, nullptr);
    if (result < 0) {
        return;
    }

    m_Frame = av_frame_alloc();
    if (!m_Frame) {
        return;
    }

    m_Packet = av_packet_alloc();
    if (!m_Packet) {
        return;
    }

    m_SwsContext = sws_getContext(m_CodecContext->width, m_CodecContext->height,
        m_CodecContext->pix_fmt, m_CodecContext->width, m_CodecContext->height,
        AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_SwsContext) {
        return;
    }

    /*m_CaptureStarted = true;
    m_CaptureThread = std::thread([&]() {
        while (m_CaptureStarted) {
            Read();
        }
    });*/
}

Video::~Video()
{
    /*if (m_CaptureThread.joinable()) {
        m_CaptureStarted = false;
        m_CaptureThread.join();
    }*/

    if (m_FormatContext)
    {
        avformat_close_input(&m_FormatContext);
    }

    if (m_CodecContext)
    {
        avcodec_free_context(&m_CodecContext);
    }

    if (m_Frame)
    {
        av_frame_free(&m_Frame);
    }

    if (m_Packet)
    {
        av_packet_free(&m_Packet);
    }

    if (m_SwsContext)
    {
        sws_freeContext(m_SwsContext);
    }
}

uint8_t* Video::GetFrame()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Buffer2.data();
}

void Video::Seek(int second) {
    if (second > m_Duration && second < m_Duration) {
        return;
    }

    m_Timestamp = (double)second / m_Timebase.num * m_Timebase.den;
    av_seek_frame(m_FormatContext, m_StreamIndex, m_Timestamp, AVSEEK_FLAG_BACKWARD);
    //std::unique_lock<std::mutex> lock(m_Mutex);
    avcodec_flush_buffers(m_CodecContext);
    //lock.unlock();
    m_WasSeek = true;
}

int Video::Read()
{
    if (m_Paused) {
        return 0;
    }

    int result = av_read_frame(m_FormatContext, m_Packet);
    if (result < 0) {
        av_packet_unref(m_Packet);
        return 0;
    }

    if (m_Packet->stream_index != m_StreamIndex) {
        av_packet_unref(m_Packet);
        return 2;
    }

    std::unique_lock<std::mutex> lock(m_Mutex);
    result = avcodec_send_packet(m_CodecContext, m_Packet);
    if (result < 0) {
        av_packet_unref(m_Packet);
        return 0;
    }

    result = avcodec_receive_frame(m_CodecContext, m_Frame);
    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
        av_packet_unref(m_Packet);
        return 0;
    }
    else if (result < 0) {
        av_packet_unref(m_Packet);
        return 0;
    }
    lock.unlock();

    double ps = m_Frame->pts * static_cast<double>(m_Timebase.num) / m_Timebase.den;

    if (m_WasSeek) {
        m_Start = std::chrono::high_resolution_clock::now() - std::chrono::seconds((long long)ps + 1);
        m_WasSeek = false;
    }

    if (m_FirstTime) {
        m_Start = std::chrono::high_resolution_clock::now();
        m_End = m_Start;
        m_FirstTime = false;
    }
    else {
        m_End = std::chrono::high_resolution_clock::now();
    }

    double time = std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Start).count() / 1000.0;
    m_CurrentTime = time;

    while (ps > time) {
        m_End = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Start).count() / 1000.0;
        double sleep_time = ps - time;
        std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
    }
    
    //std::unique_lock<std::mutex> lock2(m_Mutex);
    uint8_t* dest[4] = { m_Buffer2.data(), nullptr, nullptr, nullptr};
    int stride[4] = { m_Frame->width * 4, 0, 0, 0 };
    sws_scale(m_SwsContext, m_Frame->data, m_Frame->linesize, 0, m_Frame->height, dest, stride);
    //lock2.unlock();

    av_packet_unref(m_Packet);
    av_frame_unref(m_Frame);

    return 1;
}

int Video::GetWidth()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Width;
}

int Video::GetHeight()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Height;
}

int Video::GetDuration()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Duration;
}

int Video::GetCurrentSecond()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CurrentTime;
}

void Video::Pause() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (!m_Paused) {
        m_ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start);
    }
    m_Paused = true;
}

void Video::Resume() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Paused) {
        m_Start = std::chrono::high_resolution_clock::now() - m_ElapsedTime;
    }
    m_Paused = false;
}