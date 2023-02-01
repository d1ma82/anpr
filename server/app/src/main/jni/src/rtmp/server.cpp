#include "server.h"
#include "log.h"

rtmp::Server::Server(const char* server_input): server{server_input} {

    out_codec   = avcodec_find_encoder(codec_id);
    if (!out_codec) {
       LOGE("Could not find encode for %s\n", avcodec_get_name(codec_id))
       return;
    }
    out_codec_ctx = Codec { avcodec_alloc_context3(out_codec), [] (AVCodecContext* ctx) { avcodec_close(ctx); avcodec_free_context(&ctx); } };
    format = Format{[this] () {

            AVFormatContext *ctx {};
            CALL(avformat_alloc_output_context2(&ctx, nullptr, "flv", nullptr))
            return ctx;
        } (), avformat_free_context
    };
    if (!format) LOGE("No format")
}

void rtmp::Server::set_options_and_open_encoder(
    int dst_width, 
    int dst_height, 
    int fps, 
    int bitrate
) {
    const AVRational dst_fps {fps, 1};
    out_codec_ctx->codec_tag    =   0;
    out_codec_ctx->codec_id     =   codec_id;
    out_codec_ctx->codec_type   =   AVMEDIA_TYPE_VIDEO;
    out_codec_ctx->width        =   dst_width;
    out_codec_ctx->height       =   dst_height;
    out_codec_ctx->gop_size     =   12;
    out_codec_ctx->pix_fmt      =   out_pixel_fmt;
    out_codec_ctx->framerate    =   dst_fps;
    out_codec_ctx->time_base    =   av_inv_q(dst_fps);
    out_codec_ctx->bit_rate     =   bitrate;

    if (format->oformat->flags & AVFMT_GLOBALHEADER) out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    out_stream->time_base       =   out_codec_ctx->time_base;
    CALL(avcodec_parameters_from_context(out_stream->codecpar, out_codec_ctx.get()))
    AVDictionary *codec_options {nullptr};
    //av_dict_set(&codec_options, "profile", config.profile.c_str(), 0);
    CALL(av_dict_set(&codec_options, "preset", "ultrafast", 0))
    CALL(av_dict_set(&codec_options, "tune", "zerolatency", 0))
    CALL(av_dict_set(&codec_options, "crf", "0", 0))
    CALL(avcodec_open2(out_codec_ctx.get(), out_codec, &codec_options))
    av_dict_free(&codec_options);
}

bool rtmp::Server::listen() {
    
    if (!(format->oformat->flags & AVFMT_NOFILE)) {

        AVDictionary*       options {nullptr};
        CALL(av_dict_set(&options, "listen", "1", 0))
        CALL(av_dict_set(&options, "timeout", "100", 0))                            // seconds
        CALL(avio_open2(&format->pb, server, AVIO_FLAG_WRITE, nullptr, &options))
        av_dict_free(&options);
    }
    return err>=0;
}

void rtmp::Server::begin_stream(
    int dst_width, 
    int dst_height, 
    int fps, 
    int bitrate
) {
    out_stream = avformat_new_stream(format.get(), out_codec);
    if (!out_stream) {
        LOGE("Could not allocate stream\n")
        return;
    }
    set_options_and_open_encoder(dst_width, dst_height, fps, bitrate);
    out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;
    out_stream->codecpar->extradata      = out_codec_ctx->extradata;
    av_dump_format(format.get(), 0, server, 1);   

    CALL(avformat_write_header(format.get(), nullptr))
    LOGI("Stream time base = %d/%d\n", out_stream->time_base.num, out_stream->time_base.den);
}

bool rtmp::Server::encode_and_write_frame(Picture& picture) {

    AVPacket pkt {};
    CALL(av_frame_make_writable(picture.operator->()))
    
    picture->pts = frameindex++;  
    CALL(avcodec_send_frame(out_codec_ctx.get(), picture.operator->()))
    while (err>=0) {

        CALL(avcodec_receive_packet(out_codec_ctx.get(), &pkt))

        if (err==AVERROR(EAGAIN) || err==AVERROR_EOF) break;
        else if (err<0) return false;

        av_packet_rescale_ts(&pkt, out_codec_ctx->time_base, out_stream->time_base);
        pkt.stream_index = out_stream->index;
        //LOGI("Write packet %3" PRId64" (size=%5d)\n", pkt.pts, pkt.size)

        CALL(av_interleaved_write_frame(format.get(), &pkt))
        av_packet_unref(&pkt);
        if (err<0) return false;
    }
    int64_t stop_time = std::chrono::duration_cast<std::chrono::microseconds>(clk.now()-start_time).count();  //av_gettime() - start_time;
    int64_t video_packet_PTS = av_rescale_q(picture->pts, out_codec_ctx->time_base, {1, AV_TIME_BASE});
    if (stop_time < video_packet_PTS) {
        //LOGI("sleep: %ld", video_packet_PTS - stop_time)
        av_usleep(video_packet_PTS - stop_time);
    }
    return true;
}