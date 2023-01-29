#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

#include "rtmp/client.h"
#include "log.h"

typedef struct {
    int src_width, src_height;
    enum AVPixelFormat srcFormat;
    int dst_width, dst_height;
    enum AVPixelFormat dstFormat;
    int flags;
} ScalerPar;

const char* server = "rtmp://192.168.1.100/live/mystream/";

int main (int argc, char** argv) {
    
    rtmp::Client client;
    if (!client.connect(server)) {

        LOGE("Could not connect server\n")
        return 1;
    }
    LOGI("Begin play\n")
    client.play();
    
    ScalerPar params {
        client.codec_ctx->width, client.codec_ctx->height, 
        client.codec_ctx->pix_fmt, 
        client.codec_ctx->width, client.codec_ctx->height, 
        AV_PIX_FMT_BGR24, 
        SWS_BILINEAR
    };

    Scaler scaler = {[] (const ScalerPar& params)   { 
        return sws_getContext(
            params.src_width, params.src_height, params.srcFormat, 
            params.dst_width, params.dst_height, params.dstFormat, 
            params.flags, nullptr, nullptr, nullptr); 

        } (params), sws_freeContext
    };
    
    Picture yuv;
    cv::Mat image(client.codec_ctx->height, client.codec_ctx->width, CV_8UC3);
    int cvLinesizes[]  { static_cast<int>(image.step1()) };
    while (client.read(yuv)) {

        sws_scale(scaler.get(), yuv->data, yuv->linesize, 0, client.codec_ctx->height, &image.data, cvLinesizes);
        cv::imshow("VID", image);
        client.reset();
        if (cv::waitKey(10)==27) break;
    }
    cv::destroyWindow("VID");
    LOGI("End\n")
}
