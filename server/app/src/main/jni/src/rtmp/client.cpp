#include "client.h"

rtmp::Client::Client() {

    format = Format {avformat_alloc_context(), [](AVFormatContext* ctx) {avformat_close_input(&ctx); }};
    codec = avcodec_find_decoder(codec_id);
    if (!codec) {
        LOGE("No decoder %s", avcodec_get_name(codec_id))
        abort();
    }
    codec_ctx = Codec { avcodec_alloc_context3(codec), [] (AVCodecContext* ctx) { avcodec_close(ctx); avcodec_free_context(&ctx); }};
}

bool rtmp::Client::connect(const char* server) {

    AVFormatContext* ctx = format.get();
    CALL(avformat_open_input(&ctx, server, nullptr, nullptr)!=0)
    CALL(avformat_find_stream_info(ctx, nullptr))
    orientation = av_dict_get(ctx->metadata, "orientation", nullptr, 0);
    av_dump_format(ctx, 0, server, 0);
    return err>=0; 
}

void rtmp::Client::play() {

    for (int i=0; i<format->nb_streams; i++) 
        if (format->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) video_id=i;

    av_read_play(format.get());

    CALL(avcodec_parameters_to_context(codec_ctx.get(), format->streams[video_id]->codecpar))
    CALL(avcodec_open2(codec_ctx.get(), codec, nullptr))
    if (err<0) {
        LOGE("Could not open decoder\n")
        abort();
    }
    LOGI("Play %dx%d\n", codec_ctx->width, codec_ctx->height);
}

void rtmp::Client::stop() {
    av_read_pause(format.get());
}

bool rtmp::Client::read(Picture& pic) {

    if (av_read_frame(format.get(), &pkt)>=0) {

        CALL(avcodec_send_packet(codec_ctx.get(), &pkt))
        CALL(avcodec_receive_frame(codec_ctx.get(), pic.operator->()))
        if (err==AVERROR(EAGAIN)) {av_packet_unref(&pkt); read(pic); }
        return err>=0;
    } 
    else return false;
}