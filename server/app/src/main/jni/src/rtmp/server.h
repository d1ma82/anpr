#pragma once

#include <functional>
#include <memory>
#include <chrono>

#include "ffmpeg.h"

namespace rtmp {
    
using time_point = std::chrono::high_resolution_clock::time_point;
using high_resolution_clock = std::chrono::high_resolution_clock;

class Server {
public:
    int                 err             {0};
    Format              format          {nullptr};
    const AVCodec*      out_codec       {nullptr};
    AVCodecID           codec_id        {AV_CODEC_ID_H264};
    AVPixelFormat       out_pixel_fmt   {AV_PIX_FMT_YUV420P};
    AVStream*           out_stream      {nullptr};
    Codec               out_codec_ctx   {nullptr};
    const char*         server;
    int64_t frameindex = 0;
    high_resolution_clock clk;
    time_point start_time;

    Server(const char* server_input);
    void set_options_and_open_encoder(int dst_width, int dst_height, int fps, int bitrate);
    bool listen();
    void begin_stream(int dst_width, int dst_height, int fps, int bitrate);
    bool encode_and_write_frame(Picture& picture);
    ~Server() {}
};
}