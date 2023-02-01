#include "player.h"

filter::Player::Player(const char* server) {
    
    initCL();
    if (!client.connect(server)) {

        LOGE("Could not connect server\n")
        return;
    }

    LOGI("Begin play\n")
    client.play();
    viewport = {client.codec_ctx->width, client.codec_ctx->height};

    ScalerPar params {
        client.codec_ctx->width, client.codec_ctx->height, 
        client.codec_ctx->pix_fmt, 
        client.codec_ctx->width, client.codec_ctx->height, 
        AV_PIX_FMT_BGR24, 
        SWS_BILINEAR
    };

    scaler = Scaler{[] (const ScalerPar& params)   { 
        return sws_getContext(
            params.src_width, params.src_height, params.srcFormat, 
            params.dst_width, params.dst_height, params.dstFormat, 
            params.flags, nullptr, nullptr, nullptr); 

        } (params), sws_freeContext
    };

    image.create(client.codec_ctx->height, client.codec_ctx->width, CV_8UC3);
    cvLinesizes[0] = static_cast<int>(image.step1());
}

void filter::Player::apply(int) {

    if (client.read(yuv)) {

        sws_scale(scaler.get(), yuv->data, yuv->linesize, 0, client.codec_ctx->height, &image.data, cvLinesizes);
        frame_pix = image.getUMat(cv::ACCESS_READ);
        cv::flip(frame_pix, frame_pix, 0);
        client.reset();
    }
}