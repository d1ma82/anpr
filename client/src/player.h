#pragma once

#include <opencv2/core.hpp>
#include "filter.h"
#include "opencl.h"
#include "rtmp/client.h"

typedef struct {
    int src_width, src_height;
    enum AVPixelFormat srcFormat;
    int dst_width, dst_height;
    enum AVPixelFormat dstFormat;
    int flags;
} ScalerPar;

namespace filter{
class Player: public Input {
private:
    dims            viewport;
    rtmp::Client    client;
    Scaler          scaler      {nullptr};
    Picture         yuv;
    cv::Mat         image;
    int             cvLinesizes[1]   {0};
    cv::UMat        frame_pix;
    
public:
    Player(const char* server);

    void apply(int);

    cv::UMat frame() const noexcept { return frame_pix; }
    dims get_viewport() const noexcept { return viewport; }
    ~Player() { freeCL(); }
};
}