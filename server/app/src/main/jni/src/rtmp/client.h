#pragma once

#include <functional>
#include <memory>
#include <chrono>

#include "ffmpeg.h"

namespace rtmp {

class Client {
public:
    AVPacket        pkt         {};
    Format          format      {nullptr};
    enum AVCodecID  codec_id    {AV_CODEC_ID_H264};
    const AVCodec*  codec       {nullptr};
    Codec           codec_ctx   {nullptr};
    int             video_id    {0};
    int             err         {0};

    Client();
    bool connect(const char* server_input);
    void play();
    void stop();
    bool read(Picture& pic);
    void reset() {av_packet_unref(&pkt);}
    ~Client() {}
};
}