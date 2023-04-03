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

    int channels, sampleRate;
    AVCodecParameters* codecParameters = nullptr;
    for (int i = 0; i < m_FormatContext->nb_streams; i++) {
        AVStream* stream = m_FormatContext->streams[i];
        // stream->codecpar->channels;
        // stream->codecpar->sample_rate;
        codecParameters = stream->codecpar;
        m_Codec = avcodec_find_decoder(codecParameters->codec_id);

        if (!m_Codec) {
            continue;
        }

        if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_StreamIndex = i;
            m_Numerator = stream->time_base.num;
            m_Denominator = stream->time_base.den;
            m_Timebase = stream->time_base;
            break;
        }

    }

    if (m_StreamIndex == -1) {
        return;
    }

    m_CodecContext = avcodec_alloc_context3(m_Codec);
    if (!m_CodecContext) {
        return;
    }

    result = avcodec_parameters_to_context(m_CodecContext, codecParameters);
    if (result < 0) {
        return;
    }

    result = avcodec_open2(m_CodecContext, m_Codec, nullptr);
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
}

Video::~Video()
{
    avformat_close_input(&m_FormatContext);
    avcodec_free_context(&m_CodecContext);
    av_frame_free(&m_Frame);
    av_packet_free(&m_Packet);
}

void Video::Seek(int second) {
    if (second > m_Duration && second < m_Duration) {
        return;
    }

    int64_t timestamp = (double)second / m_Numerator * m_Denominator;
    av_seek_frame(m_FormatContext, m_StreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m_CodecContext);
    m_WasSeek = true;
}

uint8_t* Video::Read() {
    if (m_Paused) {
        return 0;
    }

    int result = av_read_frame(m_FormatContext, m_Packet);
    if (result < 0) {
        return nullptr;
    }

    if (m_Packet->stream_index != m_StreamIndex) {
        return nullptr;
    }

    result = avcodec_send_packet(m_CodecContext, m_Packet);
    if (result < 0) {
        return nullptr;
    }

    result = avcodec_receive_frame(m_CodecContext, m_Frame);
    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
        return nullptr;
    }
    else if (result < 0) {
        return nullptr;
    }

    double ps = m_Frame->pts * static_cast<double>(m_Numerator) / m_Denominator;

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
    //std::cout << "Time: " << time << std::endl;
    
    while (ps > time) {
        m_End = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::milliseconds>(m_End - m_Start).count() / 1000.0;
        av_usleep(ps - time);
    }
    
    //av_seek_frame(m_FormatContext, m_StreamIndex, a, AVSEEK_FLAG_ANY);
    //a += 3003;

    m_Buffer = new uint8_t[m_Frame->width * m_Frame->height * 4];
    SwsContext* swsContext = sws_getContext(m_Frame->width, m_Frame->height,
        m_CodecContext->pix_fmt, m_Frame->width, m_Frame->height,
        AV_PIX_FMT_RGB0, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsContext) {
        return nullptr;
    }

    uint8_t* dest[4] = { m_Buffer, nullptr, nullptr, nullptr };
    int stride[4] = { m_Frame->width * 4, 0, 0, 0 };
    sws_scale(swsContext, m_Frame->data, m_Frame->linesize, 0, m_Frame->height, dest, stride);
    sws_freeContext(swsContext);

    return m_Buffer;
}

void Video::Free() {
    delete[] m_Buffer;
}

int Video::GetWidth() const {
    return m_Frame->width;
}

int Video::GetHeight() const {
    return m_Frame->height;
}

int Video::GetDuration() const {
    return m_Duration;
}

int Video::GetCurrentSecond() const
{
    return m_CurrentTime;
}

uint8_t* Video::GetBuffer() const {
    return m_Buffer;
}

void Video::Pause() {
    if (!m_Paused) {
        m_ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start);
    }
    else {
        m_Start = std::chrono::high_resolution_clock::now() - m_ElapsedTime;
    }
    m_Paused = !m_Paused;
}

void Video::Resume() {
    if (m_Paused) {
        m_Start = std::chrono::high_resolution_clock::now() - m_ElapsedTime;
    }
    m_Paused = false;
}